#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <iostream>
#include <queue>
#include <climits>
#include <cstring>

using namespace std;
#define V 6  
const int INF = numeric_limits<int>::max();
const int SMALL_GRAPH_THRESHOLD = 100; 

int widestPath(int s, int t, const vector<vector<int>> &res, vector<int>& parent) {
    int n = res.size();
    vector<int> bottleneck(n, 0);
    // Use a max-heap where each element is (bottleneck, vertex)
    priority_queue<pair<int,int>> pq;
    bottleneck[s] = INF;
    parent.assign(n, -1);
    pq.push({bottleneck[s], s});

    while(!pq.empty()){
        int curFlow = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        // If we reached t, we can return the flow (but we still continue; 
        // in practice, the algorithm stops when t is extracted)
        if(u == t)
            return bottleneck[t];

        for (int v = 0; v < n; v++){
            if (res[u][v] > 0) {
                // The new bottleneck is the minimum of the flow to u and edge u->v capacity.
                int newFlow = min(curFlow, res[u][v]);
                if(newFlow > bottleneck[v]) {
                    bottleneck[v] = newFlow;
                    parent[v] = u;
                    pq.push({bottleneck[v], v});
                }
            }
        }
    }
    return bottleneck[t]; // if t is unreachable, bottleneck[t] remains 0.
}

// Ford-Fulkerson using the widest path (modified Dijkstra) to find augmenting paths.
int fordFulkerson(const vector<vector<int>> &capacity, int s, int t) {
    int n = capacity.size();
    // Initialize residual graph as a copy of capacity.
    vector<vector<int>> res = capacity;
    int maxFlow = 0;
    vector<int> parent(n);

    while (true) {
        // Find the augmenting path with maximum bottleneck using modified Dijkstra.
        int flow = widestPath(s, t, res, parent);
        if (flow == 0) break; // no augmenting path exists

        maxFlow += flow;

        // Trace back from t to s updating the residual capacities.
        int cur = t;
        while (cur != s) {
            int prev = parent[cur];
            res[prev][cur] -= flow;
            res[cur][prev] += flow; // add reverse edge
            cur = prev;
        }
    }
    return maxFlow;
}

bool bfs(const vector<vector<int>>& residualGraph, int source, int sink, vector<int>& parent) {
    vector<bool> visited(residualGraph.size(), false);
    queue<int> q;
    q.push(source);
    visited[source] = true;
    parent[source] = -1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (int v = 0; v < V; v++) {
            if (!visited[v] && residualGraph[u][v] > 0) {
                parent[v] = u;
                visited[v] = true;
                q.push(v);

                if (v == sink) {
                    return true;
                }
            }
        }
    }
    return false; 
}

int edmondsKarp(const vector<vector<int>>& graph, int source, int sink) {
    vector<vector<int>> residualGraph = graph;
    int n = graph.size();
    vector<int> parent(n);
    int maxFlow = 0; 
    while (bfs(residualGraph, source, sink, parent)) {
        int pathFlow = INT_MAX;

        for (int v = sink; v != source; v = parent[v]) {
            int u = parent[v];
            pathFlow = min(pathFlow, residualGraph[u][v]);
        }

        for (int v = sink; v != source; v = parent[v]) {
            int u = parent[v];
            residualGraph[u][v] -= pathFlow;
            residualGraph[v][u] += pathFlow; 
        }

        maxFlow += pathFlow;
    }

    return maxFlow;
}

// Hybrid algorithm: chooses Ford-Fulkerson with widestPath for small graphs,
// and Edmonds-Karp (BFS-based) for large graphs.
int fordFulkersonAndEdmondsKarpHybrid(const vector<vector<int>> &capacity, int s, int t) {
    int n = capacity.size();
    if(n < SMALL_GRAPH_THRESHOLD)
        return fordFulkerson(capacity, s, t);
    else
        return edmondsKarp(capacity, s, t);
}

// A simple main() to test the hybrid algorithm.
int main() {
    // Example: a small graph with 6 vertices.
    // The graph is represented as an adjacency matrix of capacities.
    // Vertices: 0 = source, 5 = sink.
    vector<vector<int>> capacity = {
        {0, 16, 13, 0, 0, 0},
        {0, 0, 10, 12, 0, 0},
        {0, 4, 0, 0, 14, 0},
        {0, 0, 9, 0, 0, 20},
        {0, 0, 0, 7, 0, 4},
        {0, 0, 0, 0, 0, 0}
    };

    int source = 0;
    int sink = 5;

    int maxFlow = fordFulkersonAndEdmondsKarpHybrid(capacity, source, sink);
    cout << "The maximum possible flow is " << maxFlow << endl;
    return 0;
}
