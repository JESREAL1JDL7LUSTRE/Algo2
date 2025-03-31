#include <bits/stdc++.h>
using namespace std;
#define INF INT_MAX

// Push-Relabel Algorithm
class PushRelabel {
    struct Edge {
        int v, flow, cap, rev;
    };
    int V;
    vector<vector<Edge>> adj;
    vector<int> height, excess;
    
public:
    PushRelabel(int V) : V(V), adj(V), height(V), excess(V) {}

    void addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    void push(int u, Edge &e) {
        int flow = min(excess[u], e.cap - e.flow);
        e.flow += flow;
        adj[e.v][e.rev].flow -= flow;
        excess[u] -= flow;
        excess[e.v] += flow;
    }

    void relabel(int u) {
        int minHeight = INF;
        for (auto &e : adj[u]) {
            if (e.flow < e.cap)
                minHeight = min(minHeight, height[e.v]);
        }
        height[u] = minHeight + 1;
    }

    int maxFlow(int s, int t) {
        height[s] = V;
        excess[s] = INF;
        for (auto &e : adj[s]) push(s, e);
        bool active = true;
        while (active) {
            active = false;
            for (int u = 0; u < V; u++) {
                if (u != s && u != t && excess[u] > 0) {
                    for (auto &e : adj[u]) {
                        if (excess[u] > 0 && e.flow < e.cap && height[u] == height[e.v] + 1)
                            push(u, e);
                    }
                    if (excess[u] > 0) {
                        relabel(u);
                        active = true;
                    }
                }
            }
        }
        return excess[t];
    }
};


int main() {
    int V = 6;
    PushRelabel pr(V);
    pr.addEdge(0, 1, 16);
    pr.addEdge(0, 2, 13);
    pr.addEdge(1, 2, 10);
    pr.addEdge(1, 3, 12);
    pr.addEdge(2, 1, 4);
    pr.addEdge(2, 4, 14);
    pr.addEdge(3, 2, 9);
    pr.addEdge(3, 5, 20);
    pr.addEdge(4, 3, 7);
    pr.addEdge(4, 5, 4);
    
    cout << "Max Flow (Push-Relabel Algorithm): " << pr.maxFlow(0, 5) << endl;
    return 0;
}