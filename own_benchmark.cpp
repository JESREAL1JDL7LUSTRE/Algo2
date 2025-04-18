#include "graphloader.h"
#include "ownAlgo.h"
#include <iostream>
#include <chrono>
#include "memory_counter.h"
using namespace std::chrono;

int main() {
    Graph graph;

    // Load graph from JSON file first
    if (!load_graph_from_json("graph700k1.2M.json", graph)) {
        std::cerr << "Failed to load graph from JSON." << std::endl;
        return 1;
    }

    std::cout << "Graph loaded with " << graph.size() << " vertices." << std::endl;

    // Now it's safe to initialize Dinic with correct size
    Dinic dinic(graph.size());

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


// int main() {
//     srand(time(0)); // Seed for random graph generation.
//     int V = 10000; // Example: Graph with 10,000 nodes.
//     Dinic dinic(V);
//     std::cout << "Number of nodes: " << V << std::endl;

//     // Build a sample graph with random capacities.
//     for (int i = 0; i < V - 1; i++) {
//         dinic.addEdge(i, i + 1, rand() % 50 + 20); // Random capacity between 20 and 70.
//         if (i + 2 < V)
//             dinic.addEdge(i, i + 2, rand() % 50 + 20);
//     }

//     std::cout << "Using " << NUM_THREADS << " threads for parallel BFS and experimental parallel DFS." << std::endl;
//     int max_flow = dinic.maxFlow(0, V - 1);  // Source is node 0, sink is node V-1.
//     std::cout << "Max Flow (Parallel BFS and DFS): " << max_flow << std::endl;

//     return 0;
// }

