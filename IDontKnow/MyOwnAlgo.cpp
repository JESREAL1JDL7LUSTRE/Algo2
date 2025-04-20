#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <climits>
#include <functional>
#include <stack>
#include <chrono>
#include "graphloader.h"
#include "memory_counter.h"

using namespace std;
using namespace std::chrono;

#define INF INT_MAX
#define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };

    int V;
    vector<vector<Edge>> adj;
    vector<int> level;
    mutex level_mutex;
    mutex update_mutex;

    // BFS worker for parallel level graph construction
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

    // Build level graph in parallel
    bool parallelBFS(int s, int t) {
        fill(level.begin(), level.end(), -1);
        vector<int> frontier = {s};
        level[s] = 0;

        vector<vector<int>> local_frontiers(NUM_THREADS);
        for (auto &lf : local_frontiers)
            lf.reserve(V / max(1, NUM_THREADS) + 10);

        while (!frontier.empty()) {
            int f_size = frontier.size();
            int num_threads = min(NUM_THREADS, max(1, f_size / 500));
            for (auto &lf : local_frontiers) lf.clear();

            vector<thread> threads;
            int chunk = (f_size + num_threads - 1) / num_threads;
            for (int i = 0; i < num_threads; i++) {
                int st = i*chunk;
                int ed = min((i+1)*chunk, f_size);
                if (st < ed)
                    threads.emplace_back(&Dinic::bfs_worker, this, cref(frontier),
                                         st, ed, ref(local_frontiers), i);
            }
            for (auto &th : threads) th.join();

            vector<int> next;
            next.reserve(f_size);
            for (auto &lf : local_frontiers)
                next.insert(next.end(), lf.begin(), lf.end());
            frontier.swap(next);
        }
        return level[t] != -1;
    }

    // Experimental parallel DFS (iterative) to push one augmenting path
    int parallelDFS(int s, int t, int flow_cap) {
        atomic<int> resultFlow(0);
        atomic<bool> found(false);

        vector<thread> threads;
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back([&]() {
                // Local pointer array to remember the next edge to try for each node.
                vector<int> local_ptr(V, 0);
                // Stacks to simulate recursion:
                // 'path' stores the sequence of nodes in the current DFS path.
                // 'edge_index' stores the chosen edge index from the parent that led to the current node.
                // 'path_flow' stores the bottleneck flow along the current path.
                vector<int> path;
                vector<int> edge_index;
                vector<int> path_flow;

                // Initialize DFS with source node.
                path.push_back(s);
                edge_index.push_back(-1); // no edge led to s
                path_flow.push_back(flow_cap);

                while (!path.empty() && !found.load()) {
                    int u = path.back();
                    if (u == t) {
                        // Found an augmenting path.
                        int pushed = path_flow.back();
                        {
                            lock_guard<mutex> lock(update_mutex);
                            // Walk the path and update flows.
                            int cur = s;
                            for (size_t j = 1; j < path.size(); j++) {
                                int v = path[j];
                                int idx = edge_index[j];
                                adj[cur][idx].flow += pushed;
                                adj[v][adj[cur][idx].rev].flow -= pushed;
                                cur = v;
                            }
                        }
                        found.store(true);
                        resultFlow.store(pushed);
                        break;
                    }

                    // Try to advance from u.
                    if (local_ptr[u] < (int)adj[u].size()) {
                        auto &e = adj[u][local_ptr[u]];
                        // Check if edge is eligible.
                        if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                            // Advance along this edge.
                            int new_flow = min(path_flow.back(), e.cap - e.flow);
                            path.push_back(e.v);
                            edge_index.push_back(local_ptr[u]); // record chosen edge
                            path_flow.push_back(new_flow);
                            // Increment pointer for node u (so that next time we try a different edge).
                            local_ptr[u]++;
                            continue;
                        } else {
                            local_ptr[u]++;
                            continue;
                        }
                    } else {
                        // No more edges from u; backtrack.
                        path.pop_back();
                        if (!edge_index.empty()) edge_index.pop_back();
                        if (!path_flow.empty()) path_flow.pop_back();
                    }
                }
            });
        }
    
        for (auto &th : threads)
            th.join();
    
        return resultFlow.load();
    }

public:
    Dinic(int V) : V(V), adj(V), level(V, -1) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0,   (int)adj[u].size() - 1});
    }

    int maxFlow(int s, int t) {
        int flow = 0;
        while (parallelBFS(s, t)) {
            // repeatedly find augmenting paths in parallel
            while (int pushed = parallelDFS(s, t, INF))
                flow += pushed;
        }
        return flow;
    }
};

int main() {
    Graph graph;
    if (!load_graph_from_json("SG.json", graph)) {
        cerr << "Failed to load graph from JSON.\n";
        return 1;
    }
    cout << "Graph loaded with " << graph.size() << " vertices.\n";

    Dinic dinic(graph.size());
    for (auto &p : graph) {
        int u = p.first;
        for (auto &e : p.second)
            dinic.addEdge(u, e.to, e.capacity);
    }

    auto start = high_resolution_clock::now();
    printMemoryUsage();
    int maxFlow = dinic.maxFlow(0, graph.size() - 1);
    printMemoryUsage();
    cout << "Max Flow using OWN algo: " << maxFlow << "\n";

    auto stop = high_resolution_clock::now();
    auto duration_mic = duration_cast<microseconds>(stop - start);
    cout << "Time taken: " << duration_mic.count() << " microseconds" << endl;
    
    auto duration_mil = duration_cast<milliseconds>(stop - start);
    cout << "Time taken: " << duration_mil.count() << " milliseconds" << endl;
    
    auto duration_sec = duration_cast<seconds>(stop - start);
    cout << "Time taken: " << duration_sec.count() << " seconds" << endl;
    

    return 0;
}
