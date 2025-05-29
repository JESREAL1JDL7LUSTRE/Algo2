#ifndef DINIC_H
#define DINIC_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <limits>
#include <algorithm>

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
                    vector<vector<int>>& local_frontiers, int thread_id);

    // Parallel BFS that partitions the current frontier among threads.
    bool parallelBFS(int s, int t);

    // Sequential DFS to find augmenting paths and compute flow.
    int dfs(int u, int t, int flow);

public:
    // Constructor for Dinic class
    Dinic(int V);

    // Add an edge from u to v with a given capacity.
    void addEdge(int u, int v, int cap);

    // Max Flow computation using the Parallel BFS and DFS approach.
    int maxFlow(int s, int t);
};

#endif // DINIC_H
