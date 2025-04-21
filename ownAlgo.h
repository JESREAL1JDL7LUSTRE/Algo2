#ifndef OWN_ALGO_H
#define OWN_ALGO_H
#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <climits>
#include <stack>

#define INF INT_MAX
// NUM_THREADS static_cast<int>(std::thread::hardware_concurrency())
#define NUM_THREADS 4 // For testing purposes, set to 2 threads

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    std::vector<std::vector<Edge>> adj;
    std::vector<int> level;
    std::mutex level_mutex;
    std::mutex update_mutex;

public:
    Dinic(int V);
    void addEdge(int u, int v, int cap);
    bool parallelBFS(int s, int t);
    int parallelDFS(int s, int t, int flow);
    int maxFlow(int s, int t);

private:
    void bfs_worker(const std::vector<int>& frontier, int start, int end,
                    std::vector<std::vector<int>>& local_frontiers, int thread_id);
};

#endif // OWN_ALGO_H


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
