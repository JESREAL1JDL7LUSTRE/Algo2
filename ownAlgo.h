// #ifndef OWN_ALGO_H
// #define OWN_ALGO_H
// #include <vector>
// #include <queue>
// #include <deque>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <algorithm>
// #include <climits>
// #include <stack>

// #define INF INT_MAX
// #define NUM_THREADS static_cast<int>(thread::hardware_concurrency())
// //define NUM_THREADS 4
// #define NUM_THREADS_DFS 2

// class Dinic {
//     struct Edge {
//         int v, flow, cap, rev;
//     };
//     int V;
//     std::vector<std::vector<Edge>> adj;
//     std::vector<int> level;
//     std::vector<int> ptr;
//     std::mutex level_mutex;
//     std::mutex update_mutex;

// public:
//     Dinic(int V);
//     void addEdge(int u, int v, int cap);
//     bool parallelBFS(int s, int t);
//     int parallelDFS(int s, int t, int flow);
//     int maxFlow(int s, int t);

// private:
//     void bfs_worker(const std::vector<int>& frontier, int start, int end,
//                     std::vector<std::vector<int>>& local_frontiers, int thread_id);
// };

// #endif // OWN_ALGO_H


// #ifndef OWN_ALGO_H
// #define OWN_ALGO_H

// #include <vector>
// #include <mutex>

// const int INF = INT_MAX;
// #define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

// class Dinic {
//     struct Edge {
//         int v;        // Destination
//         int flow;     // Current flow
//         int cap;      // Capacity
//         int rev;      // Index of reverse edge
//     };
    
// public:
//     int V;
//     std::vector<std::vector<Edge>> adj;
//     std::vector<int> level, ptr;
//     std::mutex level_mutex;

//     Dinic(int V);
//     void addEdge(int u, int v, int cap);
//     bool parallelBFS(int s, int t);
//     void bfs_worker(const std::vector<int>& frontier, int start, int end,
//                     std::vector<std::vector<int>>& local_frontiers, int thread_id);
//     int parallelDFS(int s, int t, int flow_limit);
//     int maxFlow(int s, int t);
// };

// #endif


//____________workingAlgo.h_____________

// #ifndef OWN_ALGO_H
// #define OWN_ALGO_H

// #include <vector>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <climits>
// #include <algorithm>

// #define INF INT_MAX
// #define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

// class Dinic {
//     struct Edge {
//         int v, flow, cap, rev;
//     };

//     int V;
//     std::vector<std::vector<Edge>> adj;
//     std::vector<int> level;
//     std::vector<int> ptr;

//     std::mutex level_mutex;
//     std::vector<std::mutex> node_locks;

// public:
//     Dinic(int V);
//     void addEdge(int u, int v, int cap);
//     bool parallelBFS(int s, int t);
//     int maxFlow(int s, int t);


// private:
//     void bfs_worker(const std::vector<int>& frontier, int start, int end,
//                     std::vector<std::vector<int>>& local_frontiers, int thread_id);
//     int dfs(int u, int t, int flow, std::vector<int>& local_ptr);
//     void parallelDFS_perNeighbor(int s, int t, std::atomic<int>& total_flow);
// };

// #endif // OWN_ALGO_H


// ownAlgo.h
// #ifndef OWN_ALGO_H
// #define OWN_ALGO_H

// #include <vector>
// #include <mutex>
// #include <atomic>
// #include <climits>
// #include <thread>

// #define INF INT_MAX
// #define NUM_THREADS static_cast<int>(std::thread::hardware_concurrency())

// class Dinic {
// public:
//     struct Edge {
//         int v, flow, cap, rev;
//     };

//     Dinic(int V);
//     void addEdge(int u, int v, int cap);
//     int maxFlow(int s, int t);

// private:
//     int V;
//     std::vector<std::vector<Edge>> adj;
//     std::vector<int> level;
//     std::vector<int> ptr;
//     std::vector<std::mutex> node_locks;
//     std::mutex level_mutex;

//     void bfs_worker(const std::vector<int>& frontier, int start, int end,
//                     std::vector<std::vector<int>>& local_frontiers, int thread_id);
//     bool parallelBFS(int s, int t);
//     int dfs(int u, int t, int flow, std::vector<int>& local_ptr);
//     void parallelDFS_taskQueue(int s, int t, std::atomic<int>& total_flow);
// };

// #endif // OWN_ALGO_H


// ownAlgo.h
#ifndef OPTIMIZED_ALGO_H
#define OPTIMIZED_ALGO_H

#include <vector>
#include <mutex>
#include <atomic>
#include <climits>
#include <thread>
#include <deque>
#include <memory>

#define INF INT_MAX
#define NUM_THREADS static_cast<int>(std::thread::hardware_concurrency())

class Dinic {
public:
    struct Edge {
        int v, flow, cap, rev;
    };

    struct DFSTask {
        int u;
        int edge_idx;
        int flow;
    };

    Dinic(int V);
    void addEdge(int u, int v, int cap);
    int maxFlow(int s, int t);

private:
    int V;
    std::vector<std::vector<Edge>> adj;
    std::vector<int> level;
    std::vector<int> ptr;
    std::vector<std::unique_ptr<std::mutex>> edge_locks; // Use pointers to mutexes
    
    // Sequential BFS for smaller graphs
    bool bfs(int s, int t);
    
    // Parallel BFS with optimizations
    bool parallelBFS(int s, int t);
    
    // Sequential DFS with performance optimizations
    int dfs(int u, int t, int flow);
    
    // Optimized DFS with reduced lock contention
    int dfs_optimized(int u, int t, int flow, std::vector<int>& local_ptr);
    
    // Improved parallel DFS with adaptive approach
    void parallelDFS(int s, int t, std::atomic<int>& total_flow);
};

#endif // OPTIMIZED_ALGO_H