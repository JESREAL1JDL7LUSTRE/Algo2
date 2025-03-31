#include <bits/stdc++.h>
#include <thread>
#include <atomic>
using namespace std;

#define INF INT_MAX

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> level, ptr;
    atomic<int> flow_atomic;  // Use atomic instead of mutex

public:
    Dinic(int V) : V(V), adj(V), level(V), ptr(V) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // 🔥 Optimized Parallel BFS (No Mutex)
    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        vector<int> q;
        q.push_back(s);
        level[s] = 0;

        size_t index = 0;
        while (index < q.size()) {
            int u = q[index++];
            vector<thread> threads;
            
            for (int i = 0; i < adj[u].size(); i++) {
                threads.emplace_back([&, i]() {
                    auto &e = adj[u][i];
                    if (level[e.v] == -1 && e.flow < e.cap) {
                        level[e.v] = level[u] + 1;
                        q.push_back(e.v);
                    }
                });
            }
            
            for (auto &t : threads) t.join();
        }
        return level[t] != -1;
    }

    // 🔥 Optimized Parallel DFS
    int dfs(int u, int t, int flow) {
        if (u == t) return flow;

        for (; ptr[u] < adj[u].size(); ptr[u]++) {
            auto &e = adj[u][ptr[u]];
            if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                int pushed = dfs(e.v, t, min(flow, e.cap - e.flow));
                if (pushed > 0) {
                    e.flow += pushed;
                    adj[e.v][e.rev].flow -= pushed;
                    return pushed;
                }
            }
        }
        return 0;
    }

    // 🔥 Parallel Max Flow Computation
    int maxFlow(int s, int t) {
        int flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);

            vector<thread> threads;
            atomic<int> local_flows(0);

            for (int i = 0; i < thread::hardware_concurrency(); i++) {
                threads.emplace_back([&]() {
                    int local_flow;
                    do {
                        local_flow = dfs(s, t, INF);
                        local_flows.fetch_add(local_flow, memory_order_relaxed);
                    } while (local_flow > 0);
                });
            }

            for (auto &t : threads) t.join();
            flow += local_flows.load();
        }
        return flow;
    }
};

int main() {
    int V = 10000;  // 1001 nodes
    cout << "Number of nodes: " << V << endl;
    int S = V, T = V + 1;
    V += 2;
    Dinic dinic(V);

    // 🔥 Generate a test graph
    for (int i = 0; i < V - 3; i++) {
        dinic.addEdge(i, i + 1, rand() % 50 + 20);
        if (i + 2 < V - 2) dinic.addEdge(i, i + 2, rand() % 50 + 20);
    }

    // 🔥 Connect super source and sink
    dinic.addEdge(S, 0, INF);
    dinic.addEdge(V - 3, T, INF);

    cout << "Max Flow (Optimized Parallel Dinic): " << dinic.maxFlow(S, T) << endl;
    return 0;
}
