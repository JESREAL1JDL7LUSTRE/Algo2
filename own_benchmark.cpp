#include "graphloader.h"
#include "ownAlgo.h"
#include <iostream>
#include <chrono>
#include "memory_counter.h"
using namespace std::chrono;

int main() {
    Graph graph;
    std::unordered_map<int, int> id_to_index;
    
    // Load graph from JSON file first
    if (!load_graph_from_json("SG.json", graph, id_to_index)) {
        std::cerr << "Failed to load graph from JSON." << std::endl;
        return 1;
    }

    std::cout << "Graph loaded with " << graph.size() << " vertices." << std::endl;

    // Now it's safe to initialize Dinic with correct size
    OwnDinic dinic(graph.size());

    for (const auto& pair : graph) {
        int u = pair.first;
        const std::vector<Edge>& edges_u = pair.second;

        for (const auto& edge : edges_u) {
            dinic.addEdge(u, edge.to, edge.capacity);
        }
    }
    auto start = high_resolution_clock::now();
    printMemoryUsage();
    int maxFlow = dinic.maxFlow(0, graph.size() - 1);
    printMemoryUsage();
    std::cout << "Max Flow using OWN algo: " << maxFlow << std::endl;

    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    std::cout << "Time taken: " << duration_mic.count() << " microseconds" << std::endl;
    
    auto duration_mil = duration_cast<milliseconds>(stop - start);
    std::cout << "Time taken: " << duration_mil.count() << " milliseconds" << std::endl;
    
    auto duration_sec = duration_cast<seconds>(stop - start);
    std::cout << "Time taken: " << duration_sec.count() << " seconds" << std::endl;
    
    return 0;
}
