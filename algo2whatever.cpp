#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <climits>
#include <functional>

using namespace std;

#define INF INT_MAX
#define NUM_THREADS static_cast<int>(thread::hardware_concurrency())

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> level;
    // The shared ptr array is used in sequential DFS; in parallel DFS we create thread-local copies.
    mutex level_mutex;  // Protects level updates in BFS
    mutex update_mutex; // Protects flow updates in DFS

public:
    Dinic(int V) : V(V), adj(V), level(V, -1) {}

    // Add an edge from u to v with capacity cap, and a reverse edge with 0 capacity.
    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // ---------------- Parallel BFS Worker ----------------
    // Processes a chunk of the frontier and writes discovered nodes into its local buffer.
    void bfs_worker(const vector<int>& frontier, int start, int end,
                    vector<vector<int>>& local_frontiers, int thread_id) {
        for (int i = start; i < end; i++) {
            int u = frontier[i];
            for (auto &e : adj[u]) {
                // Protect shared level updates.
                lock_guard<mutex> lock(level_mutex);
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    local_frontiers[thread_id].push_back(e.v);
                }
            }
        }
    }

    // ---------------- Parallel BFS ----------------
    bool parallelBFS(int s, int t) {
        fill(level.begin(), level.end(), -1);
        vector<int> frontier;
        frontier.push_back(s);
        level[s] = 0;

        // Create local buffers (one per thread).
        vector<vector<int>> local_frontiers(NUM_THREADS);
        for (int i = 0; i < NUM_THREADS; i++)
            local_frontiers[i].reserve(V / NUM_THREADS + 10);
        
        while (!frontier.empty()) {
            int f_size = frontier.size();
            // Use fewer threads if the frontier is small.
            int num_threads = min(NUM_THREADS, max(1, f_size / 500));
            for (auto &lf : local_frontiers)
                lf.clear();
            
            vector<thread> threads;
            int chunk_size = (f_size + num_threads - 1) / num_threads;
            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = min((i + 1) * chunk_size, f_size);
                if (start < end) {
                    threads.emplace_back(&Dinic::bfs_worker, this,
                                         cref(frontier), start, end,
                                         ref(local_frontiers), i);
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

    // ---------------- Experimental Parallel DFS ----------------
    // Each thread runs its own DFS recursively with a thread-local pointer array.
    // An atomic flag is used to indicate when an augmenting path is found.
    int parallelDFS(int s, int t, int flow) {
        atomic<int> resultFlow(0);
        atomic<bool> found(false);

        // The DFS lambda, working with a local pointer array.
        function<bool(int, int, vector<int>&)> dfs_local = [&](int u, int current_flow, vector<int> &local_ptr) -> bool {
            if (found.load()) return false; // Stop if another thread found a path.
            if (u == t) {
                // Found an augmenting path.
                lock_guard<mutex> lock(update_mutex);
                if (!found.load()) {
                    found.store(true);
                    resultFlow.store(current_flow);
                }
                return true;
            }
            // Explore all edges from u.
            for (; local_ptr[u] < (int)adj[u].size(); local_ptr[u]++) {
                auto &e = adj[u][local_ptr[u]];
                if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                    int new_flow = min(current_flow, e.cap - e.flow);
                    if (dfs_local(e.v, new_flow, local_ptr)) {
                        // Update flows under lock to avoid races.
                        lock_guard<mutex> lock(update_mutex);
                        e.flow += resultFlow.load();
                        adj[e.v][e.rev].flow -= resultFlow.load();
                        return true;
                    }
                }
            }
            return false;
        };

        vector<thread> threads;
        // Launch threads, each with its own copy of the DFS pointer array.
        for (int i = 0; i < NUM_THREADS; i++) {
            threads.emplace_back([&]() {
                vector<int> local_ptr(V, 0);
                dfs_local(s, flow, local_ptr);
            });
        }
        for (auto &th : threads)
            th.join();
        return resultFlow.load();
    }

    // ---------------- Max Flow Computation ----------------
    // Uses the parallel BFS and experimental parallel DFS.
    int maxFlow(int s, int t) {
        int flow = 0;
        while (parallelBFS(s, t)) {
            // Instead of a single shared pointer array, parallelDFS creates its own per-thread copies.
            while (int pushed = parallelDFS(s, t, INF))
                flow += pushed;
        }
        return flow;
    }
};

int main() {
    // Seed random number generator.
    srand(time(0));
    
    // Example: Build a graph with 10,000 nodes.
    int V = 10000;
    Dinic dinic(V);
    cout << "Number of nodes: " << V << endl;

    // Build a sample graph with random capacities.
    for (int i = 0; i < V - 1; i++) {
        dinic.addEdge(i, i + 1, rand() % 41 + 10);
        if (i + 2 < V)
            dinic.addEdge(i, i + 2, rand() % 41 + 10);
    }

    cout << "Using " << NUM_THREADS << " threads for parallel BFS and experimental parallel DFS." << endl;
    int max_flow = dinic.maxFlow(0, V - 1);
    cout << "Max Flow (Parallel Dinic with Parallel DFS): " << max_flow << endl;

    return 0;
}
