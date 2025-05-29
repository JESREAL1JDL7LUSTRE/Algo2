#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
#include <chrono>
#include "graphloader.h"
#include "memory_counter.h"

using namespace std;
using namespace std::chrono;

class GenericPushRelabel {
    struct Edge { int to, rev; long long cap, flow; };
    int n;
    vector<vector<Edge>> adj;
    vector<long long> excess;
    vector<int> height, ptr;
    vector<bool> active;

public:
    explicit GenericPushRelabel(int n)
        : n(n), adj(n), excess(n, 0), height(n, 0), ptr(n, 0), active(n, false) {}

    void addEdge(int u, int v, long long cap) {
        adj[u].push_back({v, (int)adj[v].size(), cap, 0});
        adj[v].push_back({u, (int)adj[u].size() - 1, 0, 0});
    }

    long long maxFlow(int s, int t) {
        fill(excess.begin(), excess.end(), 0);
        fill(height.begin(), height.end(), 0);
        fill(ptr.begin(), ptr.end(), 0);
        fill(active.begin(), active.end(), false);

        height[s] = n;
        for (auto &e : adj[s]) {
            e.flow = e.cap;
            adj[e.to][e.rev].flow = -e.cap;
            excess[e.to] += e.cap;
            excess[s] -= e.cap;
        }

        queue<int> q;
        for (int i = 0; i < n; i++) {
            if (i != s && i != t && excess[i] > 0) {
                q.push(i);
                active[i] = true;
            }
        }

        while (!q.empty()) {
            int u = q.front(); q.pop();
            active[u] = false;

            while (excess[u] > 0) {
                if (ptr[u] == (int)adj[u].size()) {
                    int min_h = INT_MAX;
                    for (auto &e : adj[u]) {
                        if (e.cap - e.flow > 0)
                            min_h = min(min_h, height[e.to]);
                    }
                    height[u] = (min_h < INT_MAX ? min_h + 1 : height[u]);
                    ptr[u] = 0;
                } else {
                    auto &e = adj[u][ptr[u]];
                    if (e.cap - e.flow > 0 && height[u] == height[e.to] + 1) {
                        long long delta = min(excess[u], e.cap - e.flow);
                        e.flow += delta;
                        adj[e.to][e.rev].flow -= delta;
                        excess[u] -= delta;
                        excess[e.to] += delta;
                        if (!active[e.to] && e.to != s && e.to != t) {
                            q.push(e.to);
                            active[e.to] = true;
                        }
                    } else {
                        ptr[u]++;
                    }
                }
            }
        }

        long long flow = 0;
        for (auto &e : adj[s]) flow += e.flow;
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

    GenericPushRelabel algo(graph.size());
    for (auto &p : graph) {
        int u = p.first;
        for (auto &edge : p.second)
            algo.addEdge(u, edge.to, edge.capacity);
    }

    auto start = high_resolution_clock::now();
    printMemoryUsage();
    long long maxFlow = algo.maxFlow(0, graph.size() - 1);
    printMemoryUsage();
    cout << "Max Flow using Push-Relabel: " << maxFlow << endl;

    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    cout << "Time taken: " << duration_mic.count() << " microseconds" << endl;
    
    auto duration_mil = duration_cast<milliseconds>(stop - start);
    cout << "Time taken: " << duration_mil.count() << " milliseconds" << endl;
    
    auto duration_sec = duration_cast<seconds>(stop - start);
    cout << "Time taken: " << duration_sec.count() << " seconds" << endl;
    
    return 0;
}
