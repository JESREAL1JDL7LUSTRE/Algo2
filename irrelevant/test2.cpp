#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

#define INF INT_MAX
#define NUM_THREADS 4 // Number of threads

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> level, ptr;

public:
    Dinic(int V) : V(V), adj(V), level(V), ptr(V) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // 🔥 Parallel BFS using OpenMP
    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        q.push(s);
        level[s] = 0;

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            // Parallelize edge traversal
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < adj[u].size(); i++) {
                auto &e = adj[u][i];
                if (level[e.v] == -1 && e.flow < e.cap) {
                    int expected = -1;
                    if (__sync_bool_compare_and_swap(&level[e.v], expected, level[u] + 1)) {
                        #pragma omp critical
                        q.push(e.v);
                    }
                }
            }
        }
        return level[t] != -1;
    }

    // 🔥 Parallel DFS using OpenMP
    int dfs(int u, int t, int flow) {
        if (u == t) return flow;
        
        for (; ptr[u] < adj[u].size(); ptr[u]++) {
            auto &e = adj[u][ptr[u]];
            if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                int pushed = 0;
                
                // Parallel DFS calls
                #pragma omp task shared(pushed)
                {
                    pushed = dfs(e.v, t, min(flow, e.cap - e.flow));
                }

                #pragma omp taskwait // Ensure all DFS tasks complete
                if (pushed > 0) {
                    #pragma omp atomic
                    e.flow += pushed;
                    #pragma omp atomic
                    adj[e.v][e.rev].flow -= pushed;
                    return pushed;
                }
            }
        }
        return 0;
    }

    // 🔥 Parallel Max Flow using DFS tasks
    int maxFlow(int s, int t) {
        int flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            
            #pragma omp parallel
            {
                int local_flow;
                do {
                    #pragma omp single nowait
                    local_flow = dfs(s, t, INF);
                    
                    #pragma omp atomic
                    flow += local_flow;
                } while (local_flow > 0);
            }
        }
        return flow;
    }
};

int main() {
    int V = 100000;
    cout << "Number of nodes: " << V << endl;
    int S = V, T = V + 1;
    V += 2;
    Dinic dinic(V);

    // 🔥 Add edges to actually create a flow network
    for (int i = 0; i < V - 3; i++) {
        dinic.addEdge(i, i + 1, rand() % 50 + 20);  // Random capacity
        if (i + 2 < V - 2) dinic.addEdge(i, i + 2, rand() % 50 + 20);
    }

    // 🔥 Connect super source and sink
    dinic.addEdge(S, 0, INF);  // Super Source → Start node
    dinic.addEdge(V - 3, T, INF);  // Last node → Super Sink

    cout << "Max Flow (Optimized Parallel Dinic): " << dinic.maxFlow(S, T) << endl;
    return 0;
}

