
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