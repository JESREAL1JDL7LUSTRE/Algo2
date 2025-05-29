
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