#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cuda_runtime.h>

using namespace std;

#define INF INT_MAX
#define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

class Dinic {
public:
    struct Edge {
        int v, flow, cap, rev;
    };

    int V;
    vector<vector<Edge>> adj;
    vector<int> level, ptr;
    mutex level_mutex;

    Dinic(int V) : V(V), adj(V), level(V, -1), ptr(V, 0) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    void bfs_worker(const vector<int>& frontier, int start, int end, vector<int>& next_frontier, atomic<int>& next_index) {
        for (int i = start; i < end; i++) {
            int u = frontier[i];
            for (auto &e : adj[u]) {
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    int idx = next_index.fetch_add(1);
                    next_frontier[idx] = e.v;
                }
            }
        }
    }

    bool parallelBFS(int s, int t) {
        fill(level.begin(), level.end(), -1);
        level[s] = 0;
        vector<int> frontier = {s};
        vector<int> next_frontier(V);
        atomic<int> next_index(0);

        while (!frontier.empty()) {
            int f_size = frontier.size();
            int num_threads = min(NUM_THREADS, max(1, f_size / 500));
            next_index = 0;

            vector<thread> threads;
            int chunk_size = (f_size + num_threads - 1) / num_threads;
            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = min((i + 1) * chunk_size, f_size);
                if (start < end) {
                    threads.emplace_back(&Dinic::bfs_worker, this, cref(frontier), start, end, ref(next_frontier), ref(next_index));
                }
            }
            for (auto &th : threads) th.join();
            
            frontier.assign(next_frontier.begin(), next_frontier.begin() + next_index);
        }
        return level[t] != -1;
    }

    void runParallelDFS(int* d_adj, int* d_capacity, int* d_flow, int* d_level, int* d_ptr, int s, int t);

    int maxFlow(int s, int t) {
        int flow = 0;
        while (parallelBFS(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            runParallelDFS(nullptr, nullptr, nullptr, nullptr, nullptr, s, t); // GPU Execution
        }
        return flow;
    }
};

__global__ void dfs_kernel(int* adj, int* capacity, int* flow, int* level, int* ptr, int* pushed_flow, int s, int t, int V) {
    int u = blockIdx.x * blockDim.x + threadIdx.x;
    if (u >= V || level[u] == -1) return;

    for (int i = ptr[u]; i < V; i++) {
        int v = adj[u * V + i];  // Access adjacency list
        if (level[v] == level[u] + 1 && flow[u * V + v] < capacity[u * V + v]) {
            int curr_flow = min(pushed_flow[u], capacity[u * V + v] - flow[u * V + v]);
            atomicAdd(&flow[u * V + v], curr_flow);
            atomicSub(&pushed_flow[u], curr_flow);
            atomicAdd(&pushed_flow[v], curr_flow);
        }
    }
}

void Dinic::runParallelDFS(int* d_adj, int* d_capacity, int* d_flow, int* d_level, int* d_ptr, int s, int t) {
    int V = this->V;
    int *d_pushed_flow;
    cudaMalloc(&d_pushed_flow, V * sizeof(int));
    cudaMemset(d_pushed_flow, 0, V * sizeof(int));
    
    int blockSize = 256;
    int gridSize = (V + blockSize - 1) / blockSize;
    dfs_kernel<<<gridSize, blockSize>>>(d_adj, d_capacity, d_flow, d_level, d_ptr, d_pushed_flow, s, t, V);

    cudaFree(d_pushed_flow);
}

int main() {
    srand(time(0));
    int V = 10000;
    Dinic dinic(V);
    cout << "Number of nodes: " << V << endl;

    for (int i = 0; i < V - 1; i++) {
        dinic.addEdge(i, i + 1, rand() % 41 + 10);
        if (i + 2 < V)
            dinic.addEdge(i, i + 2, rand() % 41 + 10);
    }

    cout << "Max Flow (Hybrid Parallel Dinic with Partitioned BFS & DFS): " << dinic.maxFlow(0, V - 1) << endl;
    return 0;
}
