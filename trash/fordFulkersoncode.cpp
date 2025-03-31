#include <iostream>
#include <limits.h>
#include <queue>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For seeding random numbers
#include <vector>

using namespace std;

#define INF INT_MAX

// Function to generate a random graph with given vertices and edge density
void generateRandomGraph(vector<vector<int>>& graph, int V, int edgeDensity) {
    srand(time(0)); // Seed for randomness

    // Initialize graph with 0s (no edges initially)
    for (int i = 0; i < V; i++) {
        for (int j = 0; j < V; j++) {
            if (i != j && (rand() % 100 < edgeDensity)) { // Add edge based on probability
                graph[i][j] = rand() % 41 + 10; // Random weight between 10-50
            } else {
                graph[i][j] = 0; // No edge
            }
        }
    }
}

// BFS function to find an augmenting path
bool bfs(vector<vector<int>>& rGraph, int s, int t, vector<int>& parent, int V) {
    vector<bool> visited(V, false);
    queue<int> q;
    
    q.push(s);
    visited[s] = true;
    parent[s] = -1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (int v = 0; v < V; v++) {
            if (!visited[v] && rGraph[u][v] > 0) { // If capacity exists and not visited
                parent[v] = u;
                visited[v] = true;
                q.push(v);

                if (v == t) return true; // Found path to sink
            }
        }
    }
    return false; // No augmenting path found
}

// Edmonds-Karp (Ford-Fulkerson using BFS)
int fordFulkerson(vector<vector<int>>& graph, int s, int t, int V) {
    vector<vector<int>> rGraph = graph; // Residual graph
    vector<int> parent(V);
    int max_flow = 0;

    while (bfs(rGraph, s, t, parent, V)) {
        int path_flow = INF;
        
        // Find the minimum capacity in the path
        for (int v = t; v != s; v = parent[v]) {
            int u = parent[v];
            path_flow = min(path_flow, rGraph[u][v]);
        }

        // Update residual capacities
        for (int v = t; v != s; v = parent[v]) {
            int u = parent[v];
            rGraph[u][v] -= path_flow;
            rGraph[v][u] += path_flow;
        }

        max_flow += path_flow;
    }

    return max_flow;
}

// Driver function
int main() {
    int V = 1000; // Number of vertices

    int edgeDensity = 1000; // Edge density (0-1000)

    vector<vector<int>> graph(V, vector<int>(V, 0)); // Initialize graph
    generateRandomGraph(graph, V, edgeDensity); // Generate a random graph

    int source = 0, sink = V - 1;
    
    cout << "The maximum possible flow is: " << fordFulkerson(graph, source, sink, V) << endl;

    return 0;
}