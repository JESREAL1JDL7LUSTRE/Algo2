// #include <iostream>
// #include <vector>
// #include <queue>
// #include <deque>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <cstdlib>
// #include <ctime>
// #include <algorithm>
// #include <climits>
// #include <functional>
// #include <stack>
// #include "ownAlgo.h"

// using namespace std;

//     Dinic::Dinic(int V) : V(V), adj(V), level(V, -1) {}

//     // Add an edge from u to v with capacity cap, and a reverse edge with 0 capacity.
//     void Dinic::addEdge(int u, int v, int cap) {
//         adj[u].push_back({v, 0, cap, (int)adj[v].size()});
//         adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
//     }

//     // ---------------- Parallel BFS Worker ----------------
//     // Processes a chunk of the frontier and writes discovered nodes into its local buffer.
//     void Dinic::bfs_worker(const vector<int>& frontier, int start, int end,
//                     vector<vector<int>>& local_frontiers, int thread_id) {
//         for (int i = start; i < end; i++) {
//             int u = frontier[i];
//             for (auto &e : adj[u]) {
//                 // Protect shared level updates.
//                 lock_guard<mutex> lock(level_mutex);
//                 if (level[e.v] == -1 && e.flow < e.cap) {
//                     level[e.v] = level[u] + 1;
//                     local_frontiers[thread_id].push_back(e.v);
//                 }
//             }
//         }
//     }

//     // ---------------- Parallel BFS ----------------
//     bool Dinic::parallelBFS(int s, int t) {
//         fill(level.begin(), level.end(), -1);
//         vector<int> frontier;
//         frontier.push_back(s);
//         level[s] = 0;

//         // Create local buffers (one per thread).
//         vector<vector<int>> local_frontiers(NUM_THREADS);
//         for (int i = 0; i < NUM_THREADS; i++)
//             local_frontiers[i].reserve(V / NUM_THREADS + 10);
        
//         while (!frontier.empty()) {
//             int f_size = frontier.size();
//             // Use fewer threads if the frontier is small.
//             int num_threads = min(NUM_THREADS, max(1, f_size / 500));
//             for (auto &lf : local_frontiers)
//                 lf.clear();
            
//             vector<thread> threads;
//             int chunk_size = (f_size + num_threads - 1) / num_threads;
//             for (int i = 0; i < num_threads; i++) {
//                 int start = i * chunk_size;
//                 int end = min((i + 1) * chunk_size, f_size);
//                 if (start < end) {
//                     threads.emplace_back(&Dinic::bfs_worker, this,
//                                          cref(frontier), start, end,
//                                          ref(local_frontiers), i);
//                 }
//             }
//             for (auto &th : threads)
//                 th.join();
            
//             vector<int> next_frontier;
//             next_frontier.reserve(f_size);
//             for (auto &lf : local_frontiers)
//                 next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());
            
//             frontier.swap(next_frontier);
//         }
//         return level[t] != -1;
//     }

//     // ---------------- Experimental Parallel DFS (Iterative) ----------------
//     // Each thread performs an iterative DFS using its own local state (including a pointer array).
//     // When a thread finds an augmenting path, it records the bottleneck flow and (under a lock)
//     // updates the flows along that path.
//     int Dinic::parallelDFS(int s, int t, int flow) {
//         atomic<int> resultFlow(0);
//         atomic<bool> found(false);

//         vector<thread> threads;
//         for (int i = 0; i < NUM_THREADS_DFS; i++) {
//             threads.emplace_back([&]() {
//                 // Local pointer array to remember the next edge to try for each node.
//                 vector<int> local_ptr(V, 0);
//                 // Stacks to simulate recursion:
//                 // 'path' stores the sequence of nodes in the current DFS path.
//                 // 'edge_index' stores the chosen edge index from the parent that led to the current node.
//                 // 'path_flow' stores the bottleneck flow along the current path.
//                 vector<int> path;
//                 vector<int> edge_index;
//                 vector<int> path_flow;

//                 // Initialize DFS with source node.
//                 path.push_back(s);
//                 edge_index.push_back(-1); // no edge led to s
//                 path_flow.push_back(flow);

//                 while (!path.empty() && !found.load()) {
//                     int u = path.back();
//                     if (u == t) {
//                         // Found an augmenting path.
//                         int pushed = path_flow.back();
//                         {
//                             lock_guard<mutex> lock(update_mutex);
//                             // Walk the path and update flows.
//                             int cur = s;
//                             for (size_t j = 1; j < path.size(); j++) {
//                                 int v = path[j];
//                                 int idx = edge_index[j];
//                                 adj[cur][idx].flow += pushed;
//                                 adj[v][adj[cur][idx].rev].flow -= pushed;
//                                 cur = v;
//                             }
//                         }
//                         found.store(true);
//                         resultFlow.store(pushed);
//                         break;
//                     }

//                     // Try to advance from u.
//                     if (local_ptr[u] < (int)adj[u].size()) {
//                         auto &e = adj[u][local_ptr[u]];
//                         // Check if edge is eligible.
//                         if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
//                             // Advance along this edge.
//                             int new_flow = min(path_flow.back(), e.cap - e.flow);
//                             path.push_back(e.v);
//                             edge_index.push_back(local_ptr[u]); // record chosen edge
//                             path_flow.push_back(new_flow);
//                             // Increment pointer for node u (so that next time we try a different edge).
//                             local_ptr[u]++;
//                             continue;
//                         } else {
//                             local_ptr[u]++;
//                             continue;
//                         }
//                     } else {
//                         // No more edges from u; backtrack.
//                         path.pop_back();
//                         if (!edge_index.empty()) edge_index.pop_back();
//                         if (!path_flow.empty()) path_flow.pop_back();
//                     }
//                 }
//             });
//         }
    
//         for (auto &th : threads)
//             th.join();
    
//         return resultFlow.load();
//     }
    
//     // ---------------- Max Flow Computation ----------------
//     // Uses the parallel BFS and experimental parallel DFS.
//     int Dinic::maxFlow(int s, int t) {
//         int flow = 0;
//         while (parallelBFS(s, t)) {
//             // Instead of a single shared pointer array, parallelDFS creates its own per-thread copies.
//             while (int pushed = parallelDFS(s, t, INF))
//                 flow += pushed;
//         }
//         return flow;
//     }

// int main() {
//     // Seed random number generator.
//     srand(time(0));
    
//     // Example: Build a graph with 10,000 nodes.
//     int V = 10000;
//     Dinic dinic(V);
//     cout << "Number of nodes: " << V << endl;

//     // Build a sample graph with random capacities.
//     for (int i = 0; i < V - 1; i++) {
//         dinic.addEdge(i, i + 1, rand() % 50 + 20);
//         if (i + 2 < V)
//             dinic.addEdge(i, i + 2, rand() % 50 + 20);
//     }

//     cout << "Using " << NUM_THREADS << " threads for parallel BFS and experimental parallel DFS." << endl;
//     int max_flow = dinic.maxFlow(0, V - 1);
//     cout << "Max Flow (Parallel BFS from Paper with Parallel and iterative DFS): " << max_flow << endl;

//     return 0;
// }

// #include <iostream>
// #include <vector>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <algorithm>
// #include <climits>
// #include "ownAlgo.h"

// using namespace std;

// // Constructor
// Dinic::Dinic(int V) : V(V), adj(V), level(V), ptr(V) {}

// // Add edge to the graph
// void Dinic::addEdge(int u, int v, int cap) {
//     adj[u].push_back({v, 0, cap, (int)adj[v].size()});
//     adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
// }

// // ====== PARALLEL BFS ======
// void Dinic::bfs_worker(const vector<int>& frontier, int start, int end,
//                        vector<vector<int>>& local_frontiers, int thread_id) {
//     for (int i = start; i < end; i++) {
//         int u = frontier[i];
//         for (auto &e : adj[u]) {
//             if (e.flow < e.cap) {
//                 lock_guard<mutex> lock(level_mutex);
//                 if (level[e.v] == -1) {
//                     level[e.v] = level[u] + 1;
//                     local_frontiers[thread_id].push_back(e.v);
//                 }
//             }
//         }
//     }
// }

// bool Dinic::parallelBFS(int s, int t) {
//     fill(level.begin(), level.end(), -1);
//     level[s] = 0;

//     vector<int> frontier = {s};
//     vector<vector<int>> local_frontiers(NUM_THREADS);
//     for (int i = 0; i < NUM_THREADS; i++)
//         local_frontiers[i].reserve(V / NUM_THREADS + 4);

//     while (!frontier.empty()) {
//         int f_size = frontier.size();
//         int num_threads = min(NUM_THREADS, max(1, f_size / 500));

//         for (auto &lf : local_frontiers) lf.clear();

//         vector<thread> threads;
//         int chunk_size = (f_size + num_threads - 1) / num_threads;

//         for (int i = 0; i < num_threads; ++i) {
//             int start = i * chunk_size;
//             int end = min((i + 1) * chunk_size, f_size);
//             if (start < end) {
//                 threads.emplace_back(&Dinic::bfs_worker, this,
//                                      cref(frontier), start, end,
//                                      ref(local_frontiers), i);
//             }
//         }

//         for (auto &th : threads) th.join();

//         vector<int> next_frontier;
//         for (auto &lf : local_frontiers)
//             next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());

//         frontier.swap(next_frontier);
//     }

//     return level[t] != -1;
// }

// // ====== SEQUENTIAL ITERATIVE DFS ======
// int Dinic::parallelDFS(int s, int t, int flow_limit) {
//     int total_flow = 0;

//     while (true) {
//         vector<int> path = {s};
//         vector<int> edge_index = {-1};
//         vector<int> path_flow = {flow_limit};

//         while (!path.empty()) {
//             int u = path.back();

//             if (u == t) {
//                 int pushed = path_flow.back();
//                 int cur = s;
//                 for (size_t j = 1; j < path.size(); ++j) {
//                     int v = path[j];
//                     int idx = edge_index[j];
//                     adj[cur][idx].flow += pushed;
//                     adj[v][adj[cur][idx].rev].flow -= pushed;
//                     cur = v;
//                 }
//                 total_flow += pushed;
//                 break;
//             }

//             while (ptr[u] < (int)adj[u].size()) {
//                 auto& e = adj[u][ptr[u]];
//                 if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
//                     int pushed = min(path_flow.back(), e.cap - e.flow);
//                     path.push_back(e.v);
//                     edge_index.push_back(ptr[u]);
//                     path_flow.push_back(pushed);
//                     ptr[u]++;
//                     goto next_step;
//                 }
//                 ptr[u]++;
//             }

//             path.pop_back();
//             if (!edge_index.empty()) edge_index.pop_back();
//             if (!path_flow.empty()) path_flow.pop_back();

//         next_step:;
//         }

//         if (path.empty()) break;
//     }

//     return total_flow;
// }

// // ====== MAX FLOW LOOP ======
// int Dinic::maxFlow(int s, int t) {
//     int flow = 0;
//     while (parallelBFS(s, t)) {
//         fill(ptr.begin(), ptr.end(), 0);
//         int pushed;
//         while ((pushed = parallelDFS(s, t, INF)) > 0) {
//             flow += pushed;
//         }
//     }
//     return flow;
// }

// __________________________WORKING PARALLEL DFS______________________

// #include <iostream>
// #include <vector>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <algorithm>
// #include <climits>
// #include <atomic>
// #include "ownAlgo.h"

// using namespace std;

// // ====== Constructor ======
// Dinic::Dinic(int V) : V(V), adj(V), level(V), ptr(V), node_locks(V) {}

// // ====== Add Edge ======
// void Dinic::addEdge(int u, int v, int cap) {
//     adj[u].push_back({v, 0, cap, (int)adj[v].size()});
//     adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
// }

// // ====== Parallel BFS Worker ======
// void Dinic::bfs_worker(const vector<int>& frontier, int start, int end,
//     vector<vector<int>>& local_frontiers, int thread_id) {
// for (int i = start; i < end; i++) {
// int u = frontier[i];
// for (auto &e : adj[u]) {
// // Use lock_guard to update shared 'level'
// lock_guard<mutex> lock(level_mutex);
// if (level[e.v] == -1 && e.flow < e.cap) {
//     level[e.v] = level[u] + 1;
//     local_frontiers[thread_id].push_back(e.v);
// }
// }
// }
// }

// // ---------------- Parallel BFS ----------------
// // This function partitions the current frontier among threads, each writes into its local buffer.
// bool Dinic::parallelBFS(int s, int t) {
// fill(level.begin(), level.end(), -1);
// vector<int> frontier;
// frontier.push_back(s);
// level[s] = 0;

// // Local buffers for each thread.
// vector<vector<int>> local_frontiers(NUM_THREADS);
// for (int i = 0; i < NUM_THREADS; i++) {
// local_frontiers[i].reserve(V / NUM_THREADS + 10);
// }

// while (!frontier.empty()) {
// int f_size = frontier.size();
// // Use fewer threads if frontier is small.
// int num_threads = min(NUM_THREADS, max(1, f_size / 500));

// for (auto &lf : local_frontiers)
// lf.clear();

// vector<thread> threads;
// int chunk_size = (f_size + num_threads - 1) / num_threads;
// for (int i = 0; i < num_threads; i++) {
// int start = i * chunk_size;
// int end = min((i + 1) * chunk_size, f_size);
// if (start < end) {
//     threads.emplace_back(&Dinic::bfs_worker, this, cref(frontier), start, end, ref(local_frontiers), i);
// }
// }
// for (auto &th : threads)
// th.join();

// vector<int> next_frontier;
// next_frontier.reserve(f_size);
// for (auto &lf : local_frontiers)
// next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());

// frontier.swap(next_frontier);
// }
// return level[t] != -1;
// }

// // ====== Thread-safe DFS from node u ======
// int Dinic::dfs(int u, int t, int flow, vector<int>& local_ptr) {
//     if (u == t) return flow;

//     for (int &i = local_ptr[u]; i < adj[u].size(); ++i) {
//         Edge &e = adj[u][i];
//         if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
//             int pushed = dfs(e.v, t, min(flow, e.cap - e.flow), local_ptr);
//             if (pushed > 0) {
//                 std::scoped_lock lock(node_locks[u], node_locks[e.v]);
//                 e.flow += pushed;
//                 adj[e.v][e.rev].flow -= pushed;
//                 return pushed;
//             }
//         }
//     }
//     return 0;
// }

// // ====== Parallel DFS Per-Neighbor ======
// void Dinic::parallelDFS_perNeighbor(int s, int t, std::atomic<int>& total_flow) {
//     vector<thread> threads;

//     for (Edge &e : adj[s]) {
//         if (level[e.v] == level[s] + 1 && e.flow < e.cap) {
//             threads.emplace_back([&]() {
//                 vector<int> local_ptr = ptr;
//                 while (true) {
//                     int pushed = dfs(e.v, t, min(INF, e.cap - e.flow), local_ptr);
//                     if (pushed == 0) break;

//                     {
//                         std::scoped_lock lock(node_locks[s], node_locks[e.v]);
//                         e.flow += pushed;
//                         adj[e.v][e.rev].flow -= pushed;
//                     }

//                     total_flow += pushed;
//                 }
//             });
//         }
//     }

//     for (auto &th : threads)
//         th.join();
// }

// // ====== Max Flow ======
// int Dinic::maxFlow(int s, int t) {
//     int flow = 0;
//     while (parallelBFS(s, t)) {
//         fill(ptr.begin(), ptr.end(), 0);
//         std::atomic<int> total_flow(0);
//         parallelDFS_perNeighbor(s, t, total_flow);
//         if (total_flow == 0) break;
//         flow += total_flow;
//     }
//     return flow;
// }


// improvingalgo.cpp - Fully Correct Parallel Dinic with Edge-Level Task Queue
// #include <iostream>
// #include <vector>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <algorithm>
// #include <climits>
// #include <atomic>
// #include "ownAlgo.h"

// using namespace std;

// Dinic::Dinic(int V) : V(V), adj(V), level(V), ptr(V), node_locks(V) {}

// void Dinic::addEdge(int u, int v, int cap) {
//     adj[u].push_back({v, 0, cap, (int)adj[v].size()});
//     adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
// }

// void Dinic::bfs_worker(const vector<int>& frontier, int start, int end,
//                        vector<vector<int>>& local_frontiers, int thread_id) {
//     for (int i = start; i < end; i++) {
//         int u = frontier[i];
//         for (auto &e : adj[u]) {
//             if (e.flow < e.cap) {
//                 lock_guard<mutex> lock(level_mutex);
//                 if (level[e.v] == -1) {
//                     level[e.v] = level[u] + 1;
//                     local_frontiers[thread_id].push_back(e.v);
//                 }
//             }
//         }
//     }
// }

// bool Dinic::parallelBFS(int s, int t) {
//     fill(level.begin(), level.end(), -1);
//     level[s] = 0;

//     vector<int> frontier = {s};
//     vector<vector<int>> local_frontiers(NUM_THREADS);
//     for (int i = 0; i < NUM_THREADS; i++)
//         local_frontiers[i].reserve(V / NUM_THREADS + 4);

//     while (!frontier.empty()) {
//         int f_size = frontier.size();
//         int num_threads = min(NUM_THREADS, max(1, f_size / 500));

//         for (auto &lf : local_frontiers) lf.clear();

//         vector<thread> threads;
//         int chunk_size = (f_size + num_threads - 1) / num_threads;

//         for (int i = 0; i < num_threads; ++i) {
//             int start = i * chunk_size;
//             int end = min((i + 1) * chunk_size, f_size);
//             if (start < end) {
//                 threads.emplace_back(&Dinic::bfs_worker, this,
//                                      cref(frontier), start, end,
//                                      ref(local_frontiers), i);
//             }
//         }

//         for (auto &th : threads) th.join();

//         vector<int> next_frontier;
//         for (auto &lf : local_frontiers)
//             next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());

//         frontier.swap(next_frontier);
//     }

//     return level[t] != -1;
// }

// int Dinic::dfs(int u, int t, int flow, vector<int>& local_ptr) {
//     if (u == t) return flow;

//     for (int &i = local_ptr[u]; i < adj[u].size(); ++i) {
//         Edge &e = adj[u][i];
//         if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
//             int pushed = dfs(e.v, t, min(flow, e.cap - e.flow), local_ptr);
//             if (pushed > 0) {
//                 std::scoped_lock lock(node_locks[u], node_locks[e.v]);
//                 e.flow += pushed;
//                 adj[e.v][e.rev].flow -= pushed;
//                 return pushed;
//             }
//         }
//     }
//     return 0;
// }

// void Dinic::parallelDFS_taskQueue(int s, int t, std::atomic<int>& total_flow) {
//     struct DFSTask {
//         int from;
//         int edge_index;
//     };

//     queue<DFSTask> workQueue;
//     mutex queue_mutex;

//     for (int i = 0; i < adj[s].size(); ++i) {
//         Edge &e = adj[s][i];
//         if (level[e.v] == level[s] + 1 && e.flow < e.cap) {
//             workQueue.push({s, i});
//         }
//     }

//     vector<thread> threads;
//     for (int i = 0; i < NUM_THREADS; ++i) {
//         threads.emplace_back([&]() {
//             while (true) {
//                 DFSTask task;
//                 {
//                     lock_guard<mutex> lock(queue_mutex);
//                     if (workQueue.empty()) return;
//                     task = workQueue.front();
//                     workQueue.pop();
//                 }

//                 int u = task.from;
//                 Edge &e = adj[u][task.edge_index];

//                 int flow_limit;
//                 {
//                     std::scoped_lock lock(node_locks[u], node_locks[e.v]);
//                     flow_limit = e.cap - e.flow;
//                 }

//                 if (flow_limit <= 0) continue;

//                 vector<int> local_ptr = ptr;
//                 int pushed = dfs(e.v, t, flow_limit, local_ptr);
//                 if (pushed > 0) {
//                     std::scoped_lock lock(node_locks[u], node_locks[e.v]);
//                     e.flow += pushed;
//                     adj[e.v][e.rev].flow -= pushed;
//                     total_flow += pushed;
//                 }
//             }
//         });
//     }
//     for (auto &th : threads) th.join();
// }

// int Dinic::maxFlow(int s, int t) {
//     int flow = 0;
//     while (parallelBFS(s, t)) {
//         fill(ptr.begin(), ptr.end(), 0);
//         std::atomic<int> total_flow(0);
//         parallelDFS_taskQueue(s, t, total_flow);
//         if (total_flow == 0) break;
//         flow += total_flow;
//     }
//     return flow;
// }


// ownAlgo.cpp
#include "ownAlgo.h"
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <algorithm>
#include <climits>
#include <atomic>
#include <deque>
#include <chrono>
#include <memory>

using namespace std;

Dinic::Dinic(int V) : V(V) {
    adj.resize(V);
    level.resize(V);
    ptr.resize(V);
    
    // Use fixed number of locks to avoid resizing a mutex vector
    int lock_count = (V / 64) + 1;
    edge_locks = vector<unique_ptr<mutex>>(lock_count);
    for (int i = 0; i < lock_count; ++i) {
        edge_locks[i] = make_unique<mutex>();
    }
}

void Dinic::addEdge(int u, int v, int cap) {
    int u_size = adj[u].size();
    int v_size = adj[v].size();
    adj[u].push_back({v, 0, cap, v_size});
    adj[v].push_back({u, 0, 0, u_size});
}

bool Dinic::bfs(int s, int t) {
    fill(level.begin(), level.end(), -1);
    level[s] = 0;
    
    queue<int> q;
    q.push(s);
    
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        
        for (const Edge& e : adj[u]) {
            if (level[e.v] == -1 && e.flow < e.cap) {
                level[e.v] = level[u] + 1;
                q.push(e.v);
            }
        }
    }
    
    return level[t] != -1;
}

bool Dinic::parallelBFS(int s, int t) {
    // For smaller graphs, use sequential BFS
    if (V < 50000) {
        return bfs(s, t);
    }
    
    fill(level.begin(), level.end(), -1);
    level[s] = 0;

    vector<int> frontier = {s};
    
    while (!frontier.empty() && level[t] == -1) {
        int f_size = frontier.size();
        
        // Adaptive threading - only use multiple threads for large frontiers
        int num_threads = (f_size > 10000) ? 
                          min(NUM_THREADS, max(1, f_size / 1000)) : 1;
        
        vector<vector<int>> next_frontiers(num_threads);
        for (auto& nf : next_frontiers) {
            nf.reserve(f_size * 3 / num_threads);
        }
        
        vector<thread> threads;
        
        if (num_threads == 1) {
            // Sequential processing for small frontiers
            for (int u : frontier) {
                for (const Edge& e : adj[u]) {
                    if (e.flow < e.cap && level[e.v] == -1) {
                        level[e.v] = level[u] + 1;
                        next_frontiers[0].push_back(e.v);
                    }
                }
            }
        } else {
            // Parallel processing for large frontiers
            int chunk_size = (f_size + num_threads - 1) / num_threads;
            
            for (int i = 0; i < num_threads; ++i) {
                int start = i * chunk_size;
                int end = min((i + 1) * chunk_size, f_size);
                
                if (start < end) {
                    threads.emplace_back([&, i, start, end]() {
                        for (int j = start; j < end; ++j) {
                            int u = frontier[j];
                            for (const Edge& e : adj[u]) {
                                if (e.flow < e.cap) {
                                    // Use atomic operation for thread safety
                                    int oldLevel = -1;
                                    if (atomic_compare_exchange_strong(
                                            reinterpret_cast<atomic<int>*>(&level[e.v]), 
                                            &oldLevel, level[u] + 1)) {
                                        next_frontiers[i].push_back(e.v);
                                    }
                                }
                            }
                        }
                    });
                }
            }
            
            for (auto& t : threads) {
                t.join();
            }
        }
        
        // Combine next frontiers
        frontier.clear();
        for (const auto& nf : next_frontiers) {
            frontier.insert(frontier.end(), nf.begin(), nf.end());
        }
    }
    
    return level[t] != -1;
}

int Dinic::dfs(int u, int t, int flow) {
    if (u == t) return flow;
    
    for (int& i = ptr[u]; i < adj[u].size(); ++i) {
        Edge& e = adj[u][i];
        
        if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
            int curr_flow = min(flow, e.cap - e.flow);
            int temp_flow = dfs(e.v, t, curr_flow);
            
            if (temp_flow > 0) {
                e.flow += temp_flow;
                adj[e.v][e.rev].flow -= temp_flow;
                return temp_flow;
            }
        }
    }
    
    return 0;
}

int Dinic::dfs_optimized(int u, int t, int flow, vector<int>& local_ptr) {
    if (u == t) return flow;
    
    for (int& i = local_ptr[u]; i < adj[u].size(); ++i) {
        Edge& e = adj[u][i];
        
        if (level[e.v] != level[u] + 1) continue;
        
        // Try lock-free approach first
        int available = e.cap - e.flow;
        if (available <= 0) continue;
        
        int pushed = min(flow, available);
        int bottleneck = (e.v == t) ? pushed : dfs_optimized(e.v, t, pushed, local_ptr);
        
        if (bottleneck > 0) {
            // Lock only the minimum required section
            int lock_idx = u / 64;
            lock_guard<mutex> lock(*edge_locks[lock_idx]);
            
            if (e.flow + bottleneck <= e.cap) { // Verify condition still holds
                e.flow += bottleneck;
                adj[e.v][e.rev].flow -= bottleneck;
                return bottleneck;
            }
        }
    }
    
    return 0;
}

void Dinic::parallelDFS(int s, int t, atomic<int>& total_flow) {
    // Adaptive approach - use sequential for small graphs
    if (V < 100000) {
        int flow;
        while ((flow = dfs(s, t, INF)) > 0) {
            total_flow += flow;
        }
        return;
    }
    
    const int TASK_THRESHOLD = 100; // Minimum task size for parallel processing
    
    // Create task queue with initial edges from source
    vector<DFSTask> tasks;
    tasks.reserve(adj[s].size());
    
    for (int i = 0; i < adj[s].size(); ++i) {
        const Edge& e = adj[s][i];
        if (level[e.v] == level[s] + 1 && e.flow < e.cap) {
            tasks.push_back({s, i, min(INF, e.cap - e.flow)});
        }
    }
    
    if (tasks.empty()) return;
    
    // Use static partitioning for small number of tasks
    if (tasks.size() < TASK_THRESHOLD) {
        for (const auto& task : tasks) {
            vector<int> local_ptr = ptr; // Local copy for this task
            Edge& e = adj[task.u][task.edge_idx];
            int pushed = dfs_optimized(e.v, t, task.flow, local_ptr);
            
            if (pushed > 0) {
                // Update the source edge with proper locking
                int lock_idx = task.u / 64;
                lock_guard<mutex> lock(*edge_locks[lock_idx]);
                
                if (e.flow + pushed <= e.cap) {
                    e.flow += pushed;
                    adj[e.v][e.rev].flow -= pushed;
                    total_flow += pushed;
                }
            }
        }
        return;
    }
    
    // For larger task sets, use dynamic work stealing
    vector<deque<DFSTask>> thread_queues(NUM_THREADS);
    atomic<bool> work_available(true);
    vector<mutex> queue_locks(NUM_THREADS);
    
    // Distribute initial tasks evenly
    for (size_t i = 0; i < tasks.size(); ++i) {
        thread_queues[i % NUM_THREADS].push_back(tasks[i]);
    }
    
    vector<thread> threads;
    for (int tid = 0; tid < NUM_THREADS; ++tid) {
        threads.emplace_back([&, tid]() {
            vector<int> local_ptr = ptr; // Thread-local copy
            int processed_count = 0;
            
            while (work_available.load()) {
                DFSTask task;
                bool has_task = false;
                
                // Try to get task from own queue first
                {
                    lock_guard<mutex> lock(queue_locks[tid]);
                    if (!thread_queues[tid].empty()) {
                        task = thread_queues[tid].front();
                        thread_queues[tid].pop_front();
                        has_task = true;
                    }
                }
                
                // Work stealing with randomization to reduce contention
                if (!has_task) {
                    // Try random thread first, then scan sequentially
                    int random_start = rand() % NUM_THREADS;
                    
                    for (int i = 0; i < NUM_THREADS && !has_task; ++i) {
                        int target = (random_start + i) % NUM_THREADS;
                        if (target == tid) continue;
                        
                        // Try lock instead of lock_guard to reduce contention
                        if (queue_locks[target].try_lock()) {
                            if (!thread_queues[target].empty()) {
                                // Steal half the work for better load balancing
                                int steal_count = max(1, (int)thread_queues[target].size() / 2);
                                
                                for (int j = 0; j < steal_count && !thread_queues[target].empty(); ++j) {
                                    if (j == 0) {
                                        task = thread_queues[target].back();
                                        has_task = true;
                                    } else {
                                        thread_queues[tid].push_front(thread_queues[target].back());
                                    }
                                    thread_queues[target].pop_back();
                                }
                            }
                            queue_locks[target].unlock();
                            if (has_task) break;
                        }
                    }
                }
                
                if (!has_task) {
                    // Check if any work is available in any queue
                    bool any_work = false;
                    for (int i = 0; i < NUM_THREADS && !any_work; ++i) {
                        if (queue_locks[i].try_lock()) {
                            any_work = !thread_queues[i].empty();
                            queue_locks[i].unlock();
                        }
                    }
                    
                    if (!any_work) {
                        // No work left
                        work_available.store(false);
                        break;
                    }
                    
                    // Backoff strategy - short yield to reduce contention
                    this_thread::yield();
                    continue;
                }
                
                // Process the task
                Edge& e = adj[task.u][task.edge_idx];
                int v = e.v;
                
                // Fast path check without locking
                if (e.flow >= e.cap || level[v] != level[task.u] + 1) {
                    continue;
                }
                
                int curr_flow = min(task.flow, e.cap - e.flow);
                int pushed = dfs_optimized(v, t, curr_flow, local_ptr);
                
                if (pushed > 0) {
                    // Lock only when updating
                    int lock_idx = task.u / 64;
                    lock_guard<mutex> lock(*edge_locks[lock_idx]);
                    
                    // Check again after acquiring lock
                    if (e.flow + pushed <= e.cap) {
                        e.flow += pushed;
                        adj[v][e.rev].flow -= pushed;
                        total_flow += pushed;
                        
                        // Create new tasks from source after successful push
                        if (++processed_count % 5 == 0) { // Batch-create tasks more frequently
                            queue_locks[tid].lock();
                            for (int i = 0; i < adj[s].size(); ++i) {
                                const Edge& new_e = adj[s][i];
                                if (level[new_e.v] == 1 && new_e.flow < new_e.cap) {
                                    thread_queues[tid].push_back({s, i, min(INF, new_e.cap - new_e.flow)});
                                }
                            }
                            queue_locks[tid].unlock();
                        }
                    }
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int Dinic::maxFlow(int s, int t) {
    int flow = 0;
    
    // Analyze graph size to determine approach
    bool use_parallel = (V > 10000);
    
    // Run the algorithm
    while (use_parallel ? parallelBFS(s, t) : bfs(s, t)) {
        fill(ptr.begin(), ptr.end(), 0);
        
        atomic<int> total_flow(0);
        
        if (use_parallel) {
            parallelDFS(s, t, total_flow);
        } else {
            // Sequential approach for small graphs
            int curr_flow;
            while ((curr_flow = dfs(s, t, INF)) > 0) {
                total_flow += curr_flow;
            }
        }
        
        if (total_flow == 0) break;
        flow += total_flow;
    }
    
    return flow;
}