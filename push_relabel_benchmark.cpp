#include "graphloader.h"
#include "push_relabel.h"
#include <iostream>
#include <chrono>
#include "memory_counter.h"
using namespace std::chrono;

int main() {
    Graph graph;

    if (!load_graph_from_json("graph100_200.json", graph)) {
        std::cerr << "Failed to load graph from JSON." << std::endl;
        return 1;
    }

    std::cout << "Graph loaded with " << graph.size() << " vertices." << std::endl;

    GenericPushRelabel  algo(graph.size());

    for (const auto& pair : graph) {
        int u = pair.first;
        const std::vector<Edge>& edges_u = pair.second;

        for (const auto& edge : edges_u) {
            algo.addEdge(u, edge.to, edge.capacity);
        }
    }

    auto start = high_resolution_clock::now();
    printMemoryUsage();
    long long maxFlow = algo.maxFlow(0, graph.size() - 1);
    printMemoryUsage();

    std::cout << "Max Flow using Push-Relabel: " << maxFlow << std::endl;

    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    std::cout << "Time taken: " << duration_mic.count() << " microseconds" << std::endl;

    auto duration_mil = duration_cast<milliseconds>(stop - start);
    std::cout << "Time taken: " << duration_mil.count() << " milliseconds" << std::endl;

    auto duration_sec = duration_cast<seconds>(stop - start);
    std::cout << "Time taken: " << duration_sec.count() << " seconds" << std::endl;

    return 0;
}
