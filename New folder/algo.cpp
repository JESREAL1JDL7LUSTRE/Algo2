// ownAlgo.cpp - Implementation of Dinic's Maximum Flow Algorithm with parallelization
#include "ownAlgo.h"  // Include header file with class declarations
#include <iostream>   // For input/output operations
#include <vector>     // For dynamic array containers
#include <queue>      // For BFS queue operations
#include <thread>     // For parallel processing capabilities
#include <mutex>      // For thread synchronization and locks
#include <algorithm>  // For algorithms like min, max, fill
#include <climits>    // For constants like INT_MAX
#include <atomic>     // For atomic operations in multithreading
#include <deque>      // For double-ended queue in work stealing
#include <chrono>     // For timing operations (not actively used)
#include <memory>     // For smart pointers like unique_ptr

using namespace std;  // Use standard namespace to avoid std:: prefix

// Constructor: Initialize the Dinic's algorithm data structures
Dinic::Dinic(int V) : V(V) {
    adj.resize(V);      // Resize adjacency list to hold V vertices
    level.resize(V);    // Resize level array for BFS layering
    ptr.resize(V);      // Resize pointer array for DFS optimization
    
    // Create fixed number of locks to avoid resizing mutex vector during runtime
    int lock_count = (V / 64) + 1;  // Calculate number of locks (one per 64 vertices)
    edge_locks = vector<unique_ptr<mutex>>(lock_count);  // Create vector of mutex pointers
    for (int i = 0; i < lock_count; ++i) {  // Initialize each mutex
        edge_locks[i] = make_unique<mutex>();  // Create new mutex using smart pointer
    }
}

// Add edge to the flow network with capacity
void Dinic::addEdge(int u, int v, int cap) {
    int u_size = adj[u].size();  // Get current size of u's adjacency list (for reverse edge index)
    int v_size = adj[v].size();  // Get current size of v's adjacency list (for reverse edge index)
    adj[u].push_back({v, 0, cap, v_size});  // Add forward edge: destination=v, flow=0, capacity=cap, reverse_index=v_size
    adj[v].push_back({u, 0, 0, u_size});    // Add reverse edge: destination=u, flow=0, capacity=0, reverse_index=u_size
}

// Standard BFS to build level graph (layered network)
bool Dinic::bfs(int s, int t) {
    fill(level.begin(), level.end(), -1);  // Initialize all levels to -1 (unvisited)
    level[s] = 0;  // Set source level to 0
    
    queue<int> q;  // Create BFS queue
    q.push(s);     // Start BFS from source
    
    while (!q.empty()) {  // Continue until queue is empty
        int u = q.front();  // Get front vertex from queue
        q.pop();            // Remove it from queue
        
        for (const Edge& e : adj[u]) {  // Examine all edges from current vertex
            if (level[e.v] == -1 && e.flow < e.cap) {  // If vertex unvisited AND edge has residual capacity
                level[e.v] = level[u] + 1;  // Set level of destination vertex
                q.push(e.v);                // Add destination to queue for further exploration
            }
        }
    }
    
    return level[t] != -1;  // Return true if sink is reachable from source
}

// Parallel BFS implementation for large graphs
bool Dinic::parallelBFS(int s, int t) {
    // For smaller graphs, sequential BFS is more efficient due to threading overhead
    if (V < 50000) {
        return bfs(s, t);  // Use standard BFS for small graphs
    }
    
    fill(level.begin(), level.end(), -1);  // Initialize all levels to -1
    level[s] = 0;  // Set source level to 0

    vector<int> frontier = {s};  // Initialize frontier with source vertex
    
    while (!frontier.empty() && level[t] == -1) {  // Continue until frontier empty or sink reached
        int f_size = frontier.size();  // Get current frontier size
        
        // Adaptive threading - use multiple threads only for large frontiers
        int num_threads = (f_size > 10000) ? 
                          min(NUM_THREADS, max(1, f_size / 1000)) : 1;  // Calculate optimal thread count
        
        vector<vector<int>> next_frontiers(num_threads);  // Create next frontier for each thread
        for (auto& nf : next_frontiers) {  // Pre-allocate memory for each next frontier
            nf.reserve(f_size * 3 / num_threads);  // Reserve space (estimate 3x growth)
        }
        
        vector<thread> threads;  // Vector to hold thread objects
        
        if (num_threads == 1) {  // Sequential processing for small frontiers
            for (int u : frontier) {  // Process each vertex in current frontier
                for (const Edge& e : adj[u]) {  // Check all edges from current vertex
                    if (e.flow < e.cap && level[e.v] == -1) {  // If edge has capacity and vertex unvisited
                        level[e.v] = level[u] + 1;  // Set level of destination vertex
                        next_frontiers[0].push_back(e.v);  // Add to next frontier
                    }
                }
            }
        } else {  // Parallel processing for large frontiers
            int chunk_size = (f_size + num_threads - 1) / num_threads;  // Calculate work chunk size per thread
            
            for (int i = 0; i < num_threads; ++i) {  // Create threads
                int start = i * chunk_size;  // Calculate start index for this thread
                int end = min((i + 1) * chunk_size, f_size);  // Calculate end index for this thread
                
                if (start < end) {  // Only create thread if there's work to do
                    threads.emplace_back([&, i, start, end]() {  // Create thread with lambda function
                        for (int j = start; j < end; ++j) {  // Process assigned chunk of frontier
                            int u = frontier[j];  // Get vertex from frontier
                            for (const Edge& e : adj[u]) {  // Check all edges from this vertex
                                if (e.flow < e.cap) {  // If edge has residual capacity
                                    // Use atomic operation for thread-safe level assignment
                                    int oldLevel = -1;  // Expected old value (unvisited)
                                    if (atomic_compare_exchange_strong(  // Atomically compare and swap
                                            reinterpret_cast<atomic<int>*>(&level[e.v]),  // Target location
                                            &oldLevel, level[u] + 1)) {  // If old value matches, set new value
                                        next_frontiers[i].push_back(e.v);  // Add to this thread's next frontier
                                    }
                                }
                            }
                        }
                    });
                }
            }
            
            for (auto& t : threads) {  // Wait for all threads to complete
                t.join();
            }
        }
        
        // Combine next frontiers from all threads
        frontier.clear();  // Clear current frontier
        for (const auto& nf : next_frontiers) {  // Iterate through each thread's next frontier
            frontier.insert(frontier.end(), nf.begin(), nf.end());  // Append to combined frontier
        }
    }
    
    return level[t] != -1;  // Return true if sink is reachable
}

// Standard DFS to find blocking flow in current level graph
int Dinic::dfs(int u, int t, int flow) {
    if (u == t) return flow;  // If reached sink, return the flow amount
    
    for (int& i = ptr[u]; i < adj[u].size(); ++i) {  // Use pointer to avoid re-checking dead edges
        Edge& e = adj[u][i];  // Get reference to current edge
        
        if (level[e.v] == level[u] + 1 && e.flow < e.cap) {  // If edge goes to next level and has capacity
            int curr_flow = min(flow, e.cap - e.flow);  // Calculate maximum flow through this edge
            int temp_flow = dfs(e.v, t, curr_flow);     // Recursively find flow from destination
            
            if (temp_flow > 0) {  // If positive flow found
                e.flow += temp_flow;                    // Increase flow on forward edge
                adj[e.v][e.rev].flow -= temp_flow;      // Decrease flow on reverse edge
                return temp_flow;                       // Return the flow amount
            }
        }
    }
    
    return 0;  // No augmenting path found
}

// Optimized DFS with local pointers and minimal locking
int Dinic::dfs_optimized(int u, int t, int flow, vector<int>& local_ptr) {
    if (u == t) return flow;  // Base case: reached sink
    
    for (int& i = local_ptr[u]; i < adj[u].size(); ++i) {  // Use thread-local pointer array
        Edge& e = adj[u][i];  // Get reference to current edge
        
        if (level[e.v] != level[u] + 1) continue;  // Skip if not going to next level
        
        // Try lock-free approach first for better performance
        int available = e.cap - e.flow;  // Calculate available capacity
        if (available <= 0) continue;    // Skip if no capacity
        
        int pushed = min(flow, available);  // Calculate flow to push
        int bottleneck = (e.v == t) ? pushed : dfs_optimized(e.v, t, pushed, local_ptr);  // Find bottleneck
        
        if (bottleneck > 0) {  // If flow can be pushed
            // Lock only the minimum required section for thread safety
            int lock_idx = u / 64;  // Calculate which lock to use
            lock_guard<mutex> lock(*edge_locks[lock_idx]);  // Acquire lock
            
            if (e.flow + bottleneck <= e.cap) { // Verify condition still holds after locking
                e.flow += bottleneck;                   // Update forward edge flow
                adj[e.v][e.rev].flow -= bottleneck;     // Update reverse edge flow
                return bottleneck;                      // Return successful flow
            }
        }
    }
    
    return 0;  // No flow found
}

// Parallel DFS implementation using work-stealing
void Dinic::parallelDFS(int s, int t, atomic<int>& total_flow) {
    // Use sequential approach for small graphs to avoid threading overhead
    if (V < 100000) {
        int flow;  // Variable to store individual flow amounts
        while ((flow = dfs(s, t, INF)) > 0) {  // Keep finding augmenting paths
            total_flow += flow;  // Add to total flow
        }
        return;  // Exit early for small graphs
    }
    
    const int TASK_THRESHOLD = 100; // Minimum task size for efficient parallel processing
    
    // Create task queue with initial edges from source
    vector<DFSTask> tasks;  // Vector to hold initial tasks
    tasks.reserve(adj[s].size());  // Reserve space for all source edges
    
    for (int i = 0; i < adj[s].size(); ++i) {  // Iterate through all edges from source
        const Edge& e = adj[s][i];  // Get reference to edge
        if (level[e.v] == level[s] + 1 && e.flow < e.cap) {  // If edge goes to next level and has capacity
            tasks.push_back({s, i, min(INF, e.cap - e.flow)});  // Create task for this edge
        }
    }
    
    if (tasks.empty()) return;  // No work to do
    
    // Use static partitioning for small number of tasks
    if (tasks.size() < TASK_THRESHOLD) {
        for (const auto& task : tasks) {  // Process each task sequentially
            vector<int> local_ptr = ptr; // Create local copy of pointers for this task
            Edge& e = adj[task.u][task.edge_idx];  // Get reference to edge
            int pushed = dfs_optimized(e.v, t, task.flow, local_ptr);  // Find flow through this path
            
            if (pushed > 0) {  // If flow was found
                // Update the source edge with proper locking
                int lock_idx = task.u / 64;  // Calculate lock index
                lock_guard<mutex> lock(*edge_locks[lock_idx]);  // Acquire lock
                
                if (e.flow + pushed <= e.cap) {  // Verify capacity constraint
                    e.flow += pushed;                   // Update forward edge
                    adj[e.v][e.rev].flow -= pushed;     // Update reverse edge
                    total_flow += pushed;               // Add to total flow
                }
            }
        }
        return;  // Exit after processing all tasks
    }
    
    // For larger task sets, use dynamic work stealing approach
    vector<deque<DFSTask>> thread_queues(NUM_THREADS);  // Create work queue for each thread
    atomic<bool> work_available(true);  // Flag to indicate if work is still available
    vector<mutex> queue_locks(NUM_THREADS);  // Lock for each thread's queue
    
    // Distribute initial tasks evenly across threads
    for (size_t i = 0; i < tasks.size(); ++i) {
        thread_queues[i % NUM_THREADS].push_back(tasks[i]);  // Round-robin distribution
    }
    
    vector<thread> threads;  // Vector to hold worker threads
    for (int tid = 0; tid < NUM_THREADS; ++tid) {  // Create worker threads
        threads.emplace_back([&, tid]() {  // Lambda function for thread work
            vector<int> local_ptr = ptr; // Thread-local copy of pointers
            int processed_count = 0;     // Counter for processed tasks
            
            while (work_available.load()) {  // Continue while work is available
                DFSTask task;      // Current task to process
                bool has_task = false;  // Flag indicating if task was obtained
                
                // Try to get task from own queue first
                {
                    lock_guard<mutex> lock(queue_locks[tid]);  // Lock own queue
                    if (!thread_queues[tid].empty()) {  // If queue not empty
                        task = thread_queues[tid].front();  // Get front task
                        thread_queues[tid].pop_front();     // Remove it from queue
                        has_task = true;                    // Mark that we have a task
                    }
                }
                
                // Work stealing with randomization to reduce contention
                if (!has_task) {  // If no task in own queue
                    // Try random thread first, then scan sequentially
                    int random_start = rand() % NUM_THREADS;  // Random starting point
                    
                    for (int i = 0; i < NUM_THREADS && !has_task; ++i) {  // Try each thread
                        int target = (random_start + i) % NUM_THREADS;  // Calculate target thread
                        if (target == tid) continue;  // Skip own thread
                        
                        // Try lock instead of lock_guard to reduce contention
                        if (queue_locks[target].try_lock()) {  // Attempt to lock target queue
                            if (!thread_queues[target].empty()) {  // If target has work
                                // Steal half the work for better load balancing
                                int steal_count = max(1, (int)thread_queues[target].size() / 2);
                                
                                for (int j = 0; j < steal_count && !thread_queues[target].empty(); ++j) {
                                    if (j == 0) {  // First stolen task becomes current task
                                        task = thread_queues[target].back();
                                        has_task = true;
                                    } else {  // Additional tasks go to own queue
                                        thread_queues[tid].push_front(thread_queues[target].back());
                                    }
                                    thread_queues[target].pop_back();  // Remove from target queue
                                }
                            }
                            queue_locks[target].unlock();  // Release target lock
                            if (has_task) break;  // Exit if task was obtained
                        }
                    }
                }
                
                if (!has_task) {  // If still no task found
                    // Check if any work is available in any queue
                    bool any_work = false;  // Flag for global work availability
                    for (int i = 0; i < NUM_THREADS && !any_work; ++i) {  // Check each thread
                        if (queue_locks[i].try_lock()) {  // Try to check queue
                            any_work = !thread_queues[i].empty();  // Check if queue has work
                            queue_locks[i].unlock();  // Release lock
                        }
                    }
                    
                    if (!any_work) {  // If no work anywhere
                        work_available.store(false);  // Signal all threads to stop
                        break;  // Exit thread loop
                    }
                    
                    // Backoff strategy - short yield to reduce contention
                    this_thread::yield();  // Give other threads chance to run
                    continue;  // Try again
                }
                
                // Process the obtained task
                Edge& e = adj[task.u][task.edge_idx];  // Get reference to edge
                int v = e.v;  // Get destination vertex
                
                // Fast path check without locking
                if (e.flow >= e.cap || level[v] != level[task.u] + 1) {  // If edge saturated or wrong level
                    continue;  // Skip this task
                }
                
                int curr_flow = min(task.flow, e.cap - e.flow);  // Calculate available flow
                int pushed = dfs_optimized(v, t, curr_flow, local_ptr);  // Find flow from destination
                
                if (pushed > 0) {  // If flow was found
                    // Lock only when updating shared data
                    int lock_idx = task.u / 64;  // Calculate lock index
                    lock_guard<mutex> lock(*edge_locks[lock_idx]);  // Acquire lock
                    
                    // Check again after acquiring lock (double-checked locking)
                    if (e.flow + pushed <= e.cap) {  // Verify capacity constraint
                        e.flow += pushed;                   // Update forward edge
                        adj[v][e.rev].flow -= pushed;       // Update reverse edge
                        total_flow += pushed;               // Add to total flow
                        
                        // Create new tasks from source after successful push
                        if (++processed_count % 5 == 0) { // Batch-create tasks for efficiency
                            queue_locks[tid].lock();  // Lock own queue
                            for (int i = 0; i < adj[s].size(); ++i) {  // Check all source edges
                                const Edge& new_e = adj[s][i];  // Get edge reference
                                if (level[new_e.v] == 1 && new_e.flow < new_e.cap) {  // If viable edge
                                    thread_queues[tid].push_back({s, i, min(INF, new_e.cap - new_e.flow)});  // Add new task
                                }
                            }
                            queue_locks[tid].unlock();  // Release queue lock
                        }
                    }
                }
            }
        });
    }
    
    for (auto& t : threads) {  // Wait for all threads to complete
        t.join();
    }
}

// Main function to compute maximum flow
int Dinic::maxFlow(int s, int t) {
    int flow = 0;  // Initialize total flow to zero
    
    // Analyze graph size to determine whether to use parallel approach
    bool use_parallel = (V > 10000);  // Use parallel for large graphs
    
    // Run the algorithm - alternating BFS and DFS phases
    while (use_parallel ? parallelBFS(s, t) : bfs(s, t)) {  // Build level graph
        fill(ptr.begin(), ptr.end(), 0);  // Reset DFS pointers for new iteration
        
        atomic<int> total_flow(0);  // Thread-safe counter for flow in this iteration
        
        if (use_parallel) {  // Use parallel DFS for large graphs
            parallelDFS(s, t, total_flow);
        } else {  // Use sequential DFS for small graphs
            int curr_flow;  // Variable for individual path flows
            while ((curr_flow = dfs(s, t, INF)) > 0) {  // Find all blocking flows
                total_flow += curr_flow;  // Add to iteration total
            }
        }
        
        if (total_flow == 0) break;  // If no flow found, algorithm is complete
        flow += total_flow;  // Add iteration flow to total flow
    }
    
    return flow;  // Return maximum flow value
}