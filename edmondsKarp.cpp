#include <iostream>
#include <queue>
#include <climits>
#include <cstring>
using namespace std;

#define V 6  
bool bfs(int residualGraph[V][V], int source, int sink, int parent[]) {
    bool visited[V];  
    memset(visited, 0, sizeof(visited));  

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

int edmondsKarp(int graph[V][V], int source, int sink) {
    int residualGraph[V][V]; 
    for (int u = 0; u < V; u++) {
        for (int v = 0; v < V; v++) {
            residualGraph[u][v] = graph[u][v]; 
        }
    }

    int parent[V]; 
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

int main() {
    int graph[V][V] = {
        {0, 16, 13, 0, 0, 0}, 
        {0, 0, 10, 12, 0, 0}, 
        {0, 4, 0, 0, 14, 0}, 
        {0, 0, 9, 0, 0, 20}, 
        {0, 0, 0, 7, 0, 4}, 
        {0, 0, 0, 0, 0, 0}
    };

    int source = 0, sink = 5;
    cout << "The maximum possible flow is: " << edmondsKarp(graph, source, sink) << endl;

    return 0;
}
