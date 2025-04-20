#include <iostream>
#include <vector>
#include <limits>
#include <climits>
#include <algorithm>
#include <chrono>
#include "graphloader.h"
#include "memory_counter.h"

using namespace std;
using namespace std::chrono;

class CapacityScaling {
    struct Edge { int to, rev; long long cap; };
    int n;
    vector<vector<Edge>> graph;
    vector<bool> visited;

    long long dfs(int v, int t, long long f, long long min_cap) {
        if (v == t) return f;
        visited[v] = true;
        for (auto &e : graph[v]) {
            if (!visited[e.to] && e.cap >= min_cap) {
                long long d = dfs(e.to, t, min(f, e.cap), min_cap);
                if (d > 0) {
                    e.cap -= d;
                    graph[e.to][e.rev].cap += d;
                    return d;
                }
            }
        }
        return 0;
    }

public:
    explicit CapacityScaling(int n): n(n), graph(n), visited(n) {}

    void addEdge(int u, int v, long long cap) {
        graph[u].push_back({v, (int)graph[v].size(), cap});
        graph[v].push_back({u, (int)graph[u].size() - 1, 0});
    }

    long long maxFlow(int s, int t) {
        long long flow = 0, max_cap = 0;
        for (int u = 0; u < n; ++u)
            for (auto &e : graph[u])
                max_cap = max(max_cap, e.cap);

        for (long long scale = 1LL << (63 - __builtin_clzll(max_cap)); scale > 0; scale >>= 1) {
            while (true) {
                fill(visited.begin(), visited.end(), false);
                long long pushed = dfs(s, t, LLONG_MAX, scale);
                if (pushed == 0) break;
                flow += pushed;
            }
        }
        return flow;
    }
};

int main() {
    Graph graph;
    if (!load_graph_from_json("SG.json", graph)) {
        cerr << "Failed to load graph from JSON." << endl;
        return 1;
    }
    cout << "Graph loaded with " << graph.size() << " vertices." << endl;

    CapacityScaling algo(graph.size());
    for (auto &p : graph) {
        int u = p.first;
        for (auto &edge : p.second)
            algo.addEdge(u, edge.to, edge.capacity);
    }

    auto start = high_resolution_clock::now();
    printMemoryUsage();
    long long maxFlow = algo.maxFlow(0, graph.size() - 1);
    printMemoryUsage();
    cout << "Max Flow using Capacity Scaling: " << maxFlow << endl;

    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    cout << "Time taken: " << duration_mic.count() << " microseconds" << endl;
    
    auto duration_mil = duration_cast<milliseconds>(stop - start);
    cout << "Time taken: " << duration_mil.count() << " milliseconds" << endl;
    
    auto duration_sec = duration_cast<seconds>(stop - start);
    cout << "Time taken: " << duration_sec.count() << " seconds" << endl;
    
    return 0;
}
