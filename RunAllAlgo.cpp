#include "graphloader.h"
#include "harernasAlgo.h"
#include "ownAlgo.h"
#include "push_relabel.h"
#include "memory_counter.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>
#include <iomanip>

using namespace std::chrono;

// Memory info structure
struct MemoryInfo {
    double currentRAM_KB;
    double peakRAM_KB;
    double pagefile_KB;
    double privateUsage_KB;
};

// Function to get current memory info using Windows API
MemoryInfo getCurrentMemoryInfo() {
    MemoryInfo info = {0.0, 0.0, 0.0, 0.0};
    PROCESS_MEMORY_COUNTERS_EX memCounter;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memCounter, sizeof(memCounter))) {
        info.currentRAM_KB = static_cast<double>(memCounter.WorkingSetSize) / 1024.0;
        info.peakRAM_KB = static_cast<double>(memCounter.PeakWorkingSetSize) / 1024.0;
        info.pagefile_KB = static_cast<double>(memCounter.PagefileUsage) / 1024.0;
        info.privateUsage_KB = static_cast<double>(memCounter.PrivateUsage) / 1024.0;
    }
    return info;
}

struct AlgorithmResult {
    std::string algorithm_name;
    std::string json_file;
    int iteration;
    long long max_flow;
    long long time_microseconds;
    long long time_milliseconds;
    long long time_seconds;
    size_t graph_size;
    size_t total_edges;
    // Memory usage before and after
    double ram_before_kb;
    double ram_after_kb;
    double peak_ram_kb;
    double pagefile_before_kb;
    double pagefile_after_kb;
    double private_usage_before_kb;
    double private_usage_after_kb;
};

std::vector<AlgorithmResult> all_results;

template <typename Algorithm>
AlgorithmResult run_algorithm(const std::string& name, Algorithm& algo, int s, int t, 
                             const std::string& json_file, int iteration, size_t graph_size, size_t total_edges) {
    
    AlgorithmResult result;
    result.algorithm_name = name;
    result.json_file = json_file;
    result.iteration = iteration;
    result.graph_size = graph_size;
    result.total_edges = total_edges;
    
    std::cout << "\nRunning " << name << " (Iteration " << iteration << ")...\n";
    
    // Get memory usage before
    MemoryInfo mem_before = getCurrentMemoryInfo();
    result.ram_before_kb = mem_before.currentRAM_KB;
    result.pagefile_before_kb = mem_before.pagefile_KB;
    result.private_usage_before_kb = mem_before.privateUsage_KB;
    
    auto start = high_resolution_clock::now();
    long long maxFlow = algo.maxFlow(s, t);
    auto stop = high_resolution_clock::now();
    
    // Get memory usage after
    MemoryInfo mem_after = getCurrentMemoryInfo();
    result.ram_after_kb = mem_after.currentRAM_KB;
    result.peak_ram_kb = mem_after.peakRAM_KB;
    result.pagefile_after_kb = mem_after.pagefile_KB;
    result.private_usage_after_kb = mem_after.privateUsage_KB;
    
    result.max_flow = maxFlow;
    result.time_microseconds = duration_cast<microseconds>(stop - start).count();
    result.time_milliseconds = duration_cast<milliseconds>(stop - start).count();
    result.time_seconds = duration_cast<seconds>(stop - start).count();

    std::cout << "Max Flow using " << name << ": " << maxFlow << std::endl;
    std::cout << "Time taken: " << result.time_microseconds << " microseconds" << std::endl;
    std::cout << "Time taken: " << result.time_milliseconds << " milliseconds" << std::endl;
    std::cout << "Time taken: " << result.time_seconds << " seconds" << std::endl;
    
    return result;
}

void export_to_excel(const std::vector<AlgorithmResult>& results, const std::string& output_file) {
    std::ofstream file(output_file);
    if (!file.is_open()) {
        std::cerr << "Failed to create output file: " << output_file << std::endl;
        return;
    }

    // Write CSV header for time comparison table
    file << "Time Comparison Table\n";
    file << "JSON File,Own Algo (Microseconds),Harernas Algo (Microseconds),Push-Relabel (Microseconds)\n";
    
    // Group results by JSON file and iteration for time comparison
    std::map<std::pair<std::string, int>, std::map<std::string, long long>> time_data;
    for (const auto& result : results) {
        time_data[{result.json_file, result.iteration}][result.algorithm_name] = result.time_microseconds;
    }
    
    for (const auto& [file_iter, algos] : time_data) {
        file << file_iter.first << " (Iteration " << file_iter.second << "),";
        file << (algos.count("OWN Algo") ? std::to_string(algos.at("OWN Algo")) : "N/A") << ",";
        file << (algos.count("Harernas Algo") ? std::to_string(algos.at("Harernas Algo")) : "N/A") << ",";
        file << (algos.count("Push-Relabel") ? std::to_string(algos.at("Push-Relabel")) : "N/A") << "\n";
    }
    
    file << "\n\nDetailed Results\n";
    file << "Algorithm,JSON File,Iteration,Max Flow,Time (μs),Time (ms),Time (s),Graph Size,Total Edges,";
    file << "RAM Before (KB),RAM After (KB),Peak RAM (KB),Pagefile Before (KB),Pagefile After (KB),";
    file << "Private Usage Before (KB),Private Usage After (KB)\n";
    
    for (const auto& result : results) {
        file << result.algorithm_name << ","
             << result.json_file << ","
             << result.iteration << ","
             << result.max_flow << ","
             << result.time_microseconds << ","
             << result.time_milliseconds << ","
             << result.time_seconds << ","
             << result.graph_size << ","
             << result.total_edges << ","
             << std::fixed << std::setprecision(2)
             << result.ram_before_kb << ","
             << result.ram_after_kb << ","
             << result.peak_ram_kb << ","
             << result.pagefile_before_kb << ","
             << result.pagefile_after_kb << ","
             << result.private_usage_before_kb << ","
             << result.private_usage_after_kb << "\n";
    }
    
    // Memory usage comparison table
    file << "\n\nMemory Usage Summary (Before Algorithm Execution)\n";
    file << "Algorithm,JSON File,Graph Size,Avg Current RAM (KB),Avg Peak RAM (KB),Avg Pagefile (KB),Avg Private Usage (KB)\n";
    
    // Group by algorithm and json file for averages
    std::map<std::pair<std::string, std::string>, std::vector<AlgorithmResult>> grouped_results;
    for (const auto& result : results) {
        grouped_results[{result.algorithm_name, result.json_file}].push_back(result);
    }
    
    for (const auto& [key, group] : grouped_results) {
        double avg_ram_before = 0, avg_peak_ram = 0, avg_pagefile_before = 0, avg_private_before = 0;
        size_t graph_size = group[0].graph_size;
        
        for (const auto& result : group) {
            avg_ram_before += result.ram_before_kb;
            avg_peak_ram += result.peak_ram_kb;
            avg_pagefile_before += result.pagefile_before_kb;
            avg_private_before += result.private_usage_before_kb;
        }
        
        size_t count = group.size();
        avg_ram_before /= count;
        avg_peak_ram /= count;
        avg_pagefile_before /= count;
        avg_private_before /= count;
        
        file << key.first << "," << key.second << "," << graph_size << ","
             << avg_ram_before << "," << avg_peak_ram << "," 
             << avg_pagefile_before << "," << avg_private_before << "\n";
    }
    
    // Memory usage after algorithm execution
    file << "\n\nMemory Usage Summary (After Algorithm Execution)\n";
    file << "Algorithm,JSON File,Graph Size,Avg Current RAM (KB),Avg Peak RAM (KB),Avg Pagefile (KB),Avg Private Usage (KB)\n";
    
    for (const auto& [key, group] : grouped_results) {
        double avg_ram_after = 0, avg_peak_ram = 0, avg_pagefile_after = 0, avg_private_after = 0;
        size_t graph_size = group[0].graph_size;
        
        for (const auto& result : group) {
            avg_ram_after += result.ram_after_kb;
            avg_peak_ram += result.peak_ram_kb;
            avg_pagefile_after += result.pagefile_after_kb;
            avg_private_after += result.private_usage_after_kb;
        }
        
        size_t count = group.size();
        avg_ram_after /= count;
        avg_peak_ram /= count;
        avg_pagefile_after /= count;
        avg_private_after /= count;
        
        file << key.first << "," << key.second << "," << graph_size << ","
             << avg_ram_after << "," << avg_peak_ram << "," 
             << avg_pagefile_after << "," << avg_private_after << "\n";
    }
    
    file.close();
    std::cout << "\nResults exported to: " << output_file << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <iterations> <input_json1> <input_json2> ..." << std::endl;
        std::cerr << "Example: " << argv[0] << " 5 file1.json file2.json" << std::endl;
        return 1;
    }

    int iterations = std::stoi(argv[1]);
    std::vector<std::string> json_files;
    
    for (int i = 2; i < argc; i++) {
        json_files.push_back(argv[i]);
    }

    std::cout << "Running " << iterations << " iterations on " << json_files.size() << " JSON files.\n";

    for (const std::string& input_file : json_files) {
        std::cout << "\n=== Processing " << input_file << " ===" << std::endl;
        
        Graph graph;
        std::unordered_map<int, int> id_to_index;

        if (!load_graph_from_json(input_file, graph, id_to_index)) {
            std::cerr << "Failed to load graph from JSON: " << input_file << std::endl;
            continue;
        }

        std::cout << "Graph loaded with " << graph.size() << " vertices." << std::endl;

        // Calculate total number of edges
        size_t total_edges = 0;
        for (const auto& [u, edges] : graph) {
            total_edges += edges.size();
        }
        std::cout << "Total edges: " << total_edges << std::endl;
        
        auto min_node_id = std::min_element(id_to_index.begin(), id_to_index.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; })->first;
        auto max_node_id = std::max_element(id_to_index.begin(), id_to_index.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; })->first;

        int s = id_to_index[min_node_id];
        int t = id_to_index[max_node_id];

        std::cout << "Using source node ID: " << min_node_id << ", sink node ID: " << max_node_id << std::endl;

        // Run multiple iterations
        for (int iter = 1; iter <= iterations; iter++) {
            std::cout << "\n--- Iteration " << iter << " ---" << std::endl;
            
            // Run Harernas (Parallel Dinic)
            Dinic harernas(graph.size());
            for (const auto& [u, edges] : graph)
                for (const auto& edge : edges)
                    harernas.addEdge(u, edge.to, edge.capacity);
            
            AlgorithmResult result1 = run_algorithm("Harernas Algo", harernas, s, t, 
                                                   input_file, iter, graph.size(), total_edges);
            all_results.push_back(result1);

            // Run OwnAlgo (Presumably Sequential Dinic or another variant)
            OwnDinic own(graph.size());
            for (const auto& [u, edges] : graph)
                for (const auto& edge : edges)
                    own.addEdge(u, edge.to, edge.capacity);
            
            AlgorithmResult result2 = run_algorithm("OWN Algo", own, s, t, 
                                                   input_file, iter, graph.size(), total_edges);
            all_results.push_back(result2);

            // Run Push-Relabel
            ImprovedPushRelabel push_relabel(graph.size());
            for (const auto& [u, edges] : graph)
                for (const auto& edge : edges)
                    push_relabel.addEdge(u, edge.to, edge.capacity);
            
            AlgorithmResult result3 = run_algorithm("Push-Relabel", push_relabel, s, t, 
                                                   input_file, iter, graph.size(), total_edges);
            all_results.push_back(result3);
        }
    }

    // Export all results to Excel (CSV format)
    std::string output_filename = "algorithm_results.csv";
    export_to_excel(all_results, output_filename);

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Total experiments run: " << all_results.size() << std::endl;
    std::cout << "Results saved to: " << output_filename << std::endl;
    std::cout << "You can open this file in Excel or any spreadsheet application." << std::endl;

    return 0;
}