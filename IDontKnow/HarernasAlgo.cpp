#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <algorithm>
#include <queue>
#include <deque>
#include "graphloader.h"
#include "memory_counter.h"

using namespace std::chrono;
using namespace std;

#define INF INT_MAX
#define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };

    int V;
    vector<vector<Edge>> adj;
    vector<int> level, ptr;
    mutex level_mutex;  // Mutex to protect shared resources during parallel BFS

    // Parallel BFS Worker that processes a chunk of the frontier.
    void bfs_worker(const vector<int>& frontier, int start, int end,
                    vector<vector<int>>& local_frontiers, int thread_id) {
        for (int i = start; i < end; i++) {
            int u = frontier[i];
            for (auto &e : adj[u]) {
                lock_guard<mutex> lock(level_mutex);
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    local_frontiers[thread_id].push_back(e.v);
                }
            }
        }
    }

    // Parallel BFS that partitions the current frontier among threads.
    bool parallelBFS(int s, int t) {
        fill(level.begin(), level.end(), -1);
        vector<int> frontier;
        frontier.push_back(s);
        level[s] = 0;

        vector<vector<int>> local_frontiers(NUM_THREADS);
        for (int i = 0; i < NUM_THREADS; i++)
            local_frontiers[i].reserve(V / NUM_THREADS + 10);
        
        while (!frontier.empty()) {
            int f_size = frontier.size();
            int num_threads = min(NUM_THREADS, max(1, f_size / 500));
            
            for (auto &lf : local_frontiers)
                lf.clear();
            
            vector<thread> threads;
            int chunk_size = (f_size + num_threads - 1) / num_threads;
            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = min((i + 1) * chunk_size, f_size);
                if (start < end) {
                    threads.emplace_back(&Dinic::bfs_worker, this, cref(frontier),
                                         start, end, ref(local_frontiers), i);
                }
            }
            for (auto &th : threads)
                th.join();
            
            vector<int> next_frontier;
            next_frontier.reserve(f_size);
            for (auto &lf : local_frontiers)
                next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());
            
            frontier.swap(next_frontier);
        }
        return level[t] != -1;
    }

    // Sequential DFS to find augmenting paths and compute flow.
    int dfs(int u, int t, int flow) {
        if (u == t)
            return flow;
        for (int &i = ptr[u]; i < (int)adj[u].size(); i++) {
            auto &e = adj[u][i];
            if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                int pushed = dfs(e.v, t, min(flow, e.cap - e.flow));
                if (pushed > 0) {
                    e.flow += pushed;
                    adj[e.v][e.rev].flow -= pushed;
                    return pushed;
                }
            }
        }
        return 0;
    }

public:
    // Constructor for Dinic class
    Dinic(int V) : V(V), adj(V), level(V, -1), ptr(V, 0) {}

    // Add an edge from u to v with a given capacity.
    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // Max Flow computation using the Parallel BFS and DFS approach.
    int maxFlow(int s, int t) {
        int flow = 0;
        while (parallelBFS(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (int pushed = dfs(s, t, INF))
                flow += pushed;
        }
        return flow;
    }
};

int main() {
    Graph graph;

    // Load graph from JSON file first
    if (!load_graph_from_json("SG.json", graph)) {
        cerr << "Failed to load graph from JSON." << endl;
        return 1;
    }

    cout << "Graph loaded with " << graph.size() << " vertices." << endl;

    // Now it's safe to initialize Dinic with correct size
    Dinic dinic(graph.size());

    for (const auto& pair : graph) {
        int u = pair.first;
        const vector<Edge>& edges_u = pair.second;
        for (const auto& edge : edges_u) {
            dinic.addEdge(u, edge.to, edge.capacity);
        }
    }

    auto start = high_resolution_clock::now();
    printMemoryUsage();
    int maxFlow = dinic.maxFlow(0, graph.size() - 1);
    printMemoryUsage();
    cout << "Max Flow using Harernas Algo: " << maxFlow << endl;
    
    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    cout << "Time taken: " << duration_mic.count() << " microseconds" << endl;
    
    auto duration_mil = duration_cast<milliseconds>(stop - start);
    cout << "Time taken: " << duration_mil.count() << " milliseconds" << endl;
    
    auto duration_sec = duration_cast<seconds>(stop - start);
    cout << "Time taken: " << duration_sec.count() << " seconds" << endl;
    
    return 0;
}
