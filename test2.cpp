#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

#define INF INT_MAX
#define PARTITIONS 4  // Number of partitions

class Dinic {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> level;

public:
    Dinic(int V) : V(V), adj(V), level(V) {}

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
            int u = q.front();
            q.pop();
            for (auto &e : adj[u]) {
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    q.push(e.v);
                }
            }
        }
        return level[t] != -1;
    }

    int tabulation_flow(int s, int t) {
        vector<int> dp(V, 0); // dp[u] stores max flow from u to t
        queue<int> q;
        dp[s] = INF;
        q.push(s);

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            for (auto &e : adj[u]) {
                if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                    int available_flow = min(dp[u], e.cap - e.flow);
                    
                    if (available_flow > 0 && dp[e.v] == 0) {
                        dp[e.v] += available_flow;
                        e.flow += available_flow;
                        adj[e.v][e.rev].flow -= available_flow;
                        q.push(e.v);
                    }
                }
            }
        }
        return dp[t]; // Max flow to sink
    }

    int maxFlow(int s, int t) {
        int flow = 0;
        while (bfs(s, t)) {
            int pushed;
            while ((pushed = tabulation_flow(s, t)) > 0) {
                flow += pushed;
            }
        }
        return flow;
    }
};

// Partition the graph into `P` partitions
void partitionGraph(vector<vector<int>> &partitions, int V, int P) {
    int chunk_size = V / P;
    for (int i = 0; i < P; i++) {
        int start = i * chunk_size;
        int end = (i == P - 1) ? V : start + chunk_size;

        for (int j = start; j < end; j++) {
            partitions[i].push_back(j);
        }
    }
}

int main() {
    int V = 1e5 + 1;
    cout << "Number of nodes: " << V << endl;
    int S = V, T = V + 1;
    V += 2;
    Dinic dinic(V);

    vector<vector<int>> partitions(PARTITIONS);
    partitionGraph(partitions, V - 2, PARTITIONS);

    // Parallel edge addition
    #pragma omp parallel for
    for (int i = 0; i < V - 3; i++) {
        dinic.addEdge(i, i + 1, rand() % 50 + 1);
        if (i + 2 < V - 2) dinic.addEdge(i, i + 2, rand() % 50 + 1);
    }

    for (int i = 0; i < PARTITIONS; i++) {
        int start = partitions[i].front();
        int end = partitions[i].back();
        dinic.addEdge(S, start, INF);
        dinic.addEdge(end, T, INF);
    }

    cout << "Max Flow (Tabulation Dinic's Algorithm): " << dinic.maxFlow(S, T) << endl;
    return 0;
}
