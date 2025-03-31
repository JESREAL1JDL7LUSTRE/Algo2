#include <bits/stdc++.h>
#include <thread>
#include <mutex>
using namespace std;

#define INF INT_MAX
#define NUM_THREADS 4  // Adjust based on CPU cores

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> level, ptr;
    mutex flow_mutex;  // Thread-safe flow updates

public:
    Dinic(int V) : V(V), adj(V), level(V), ptr(V) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // 🔥 Parallel BFS (Same as Before)
    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        q.push(s);
        level[s] = 0;

        while (!q.empty()) {
            int qsize = q.size();
            vector<thread> threads;
            vector<queue<int>> thread_queues(NUM_THREADS);

            for (int i = 0; i < qsize; i++) {
                int u = q.front();
                q.pop();
                thread_queues[i % NUM_THREADS].push(u);
            }

            for (int i = 0; i < NUM_THREADS; i++) {
                threads.emplace_back([&, i]() {
                    while (!thread_queues[i].empty()) {
                        int u = thread_queues[i].front();
                        thread_queues[i].pop();

                        for (auto &e : adj[u]) {
                            if (level[e.v] == -1 && e.flow < e.cap) {
                                lock_guard<mutex> lock(flow_mutex);
                                level[e.v] = level[u] + 1;
                                q.push(e.v);
                            }
                        }
                    }
                });
            }

            for (auto &t : threads) t.join();
        }
        return level[t] != -1;
    }

    // 🔥 Parallel DFS using `std::thread`
    int dfs(int u, int t, int flow) {
        if (u == t) return flow;

        vector<thread> threads;
        vector<int> thread_results(NUM_THREADS, 0);

        for (; ptr[u] < adj[u].size(); ptr[u]++) {
            auto &e = adj[u][ptr[u]];
            if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                int thread_id = ptr[u] % NUM_THREADS;
                threads.emplace_back([&, thread_id]() {
                    int pushed = dfs(e.v, t, min(flow, e.cap - e.flow));

                    if (pushed > 0) {
                        lock_guard<mutex> lock(flow_mutex);  // Prevent race conditions
                        e.flow += pushed;
                        adj[e.v][e.rev].flow -= pushed;
                        thread_results[thread_id] += pushed;
                    }
                });
            }
        }

        for (auto &t : threads) t.join();

        int total_flow = 0;
        for (int f : thread_results) total_flow += f;
        return total_flow;
    }

    // 🔥 Parallel Max Flow Computation
    int maxFlow(int s, int t) {
        int flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);

            vector<thread> threads;
            vector<int> local_flows(NUM_THREADS, 0);

            for (int i = 0; i < NUM_THREADS; i++) {
                threads.emplace_back([&, i]() {
                    int local_flow;
                    do {
                        local_flow = dfs(s, t, INF);
                        local_flows[i] += local_flow;
                    } while (local_flow > 0);
                });
            }

            for (auto &t : threads) t.join();

            for (int f : local_flows) {
                flow += f;
            }
        }
        return flow;
    }
};

int main() {
    int V = 1e3 + 1;  // 100,001 nodes
    cout << "Number of nodes: " << V << endl;
    int S = V, T = V + 1;
    V += 2;
    Dinic dinic(V);

    // 🔥 Generate a large graph with random capacities
    for (int i = 0; i < V - 3; i++) {
        dinic.addEdge(i, i + 1, rand() % 50 + 1);
        if (i + 2 < V - 2) dinic.addEdge(i, i + 2, rand() % 50 + 1);
    }

    // 🔥 Connect super source and sink
    dinic.addEdge(S, 0, INF);
    dinic.addEdge(V - 3, T, INF);

    cout << "Max Flow (Parallel Dinic with C++ Threads): " << dinic.maxFlow(S, T) << endl;
    return 0;
}
