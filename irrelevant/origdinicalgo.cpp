#include <bits/stdc++.h>
using namespace std;
#define INF INT_MAX

// Dinic's Algorithm
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

    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        q.push(s);
        level[s] = 0;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (auto &e : adj[u]) {
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    q.push(e.v);
                }
            }
        }
        return level[t] != -1;
    }

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

    int maxFlow(int s, int t) {
        int flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (int pushed = dfs(s, t, INF)) {
                flow += pushed;
            }
        }
        return flow;
    }
};

int main() {
    int V = 100000;
    Dinic dinic(V);
    cout << "Number of nodes: " << V << endl;

    for (int i = 0; i < V - 1; i++) {
        dinic.addEdge(i, i + 1, rand() % 50 + 20);
        if (i + 2 < V)
            dinic.addEdge(i, i + 2, rand() % 50 + 20);
        
    }
    
    cout << "Max Flow (Dinic's Algorithm): " << dinic.maxFlow(0, V - 1) << endl;
    return 0;    
}
