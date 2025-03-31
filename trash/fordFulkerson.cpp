#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <cstdlib>  // For rand()
#include <ctime>    // For time()

using namespace std;

// Function to generate a random graph with given vertices and edge density
void generateRandomGraph(vector<vector<int>>& graph, int V, int edgeDensity) {
    srand(time(0)); // Seed for randomness

    // Initialize graph with 0s (no edges initially)
    for (int i = 0; i < V; i++) {
        for (int j = 0; j < V; j++) {
            if (i != j && (rand() % 1000 < edgeDensity)) { // Add edge based on probability
                graph[i][j] = rand() % 41 + 10; // Random weight between 10-50
            } else {
                graph[i][j] = 0; // No edge
            }
        }
    }
}

// Improved BFS that constructs a level graph
bool improvedBFS(vector<vector<int>>& rGraph, int s, int t, vector<int>& parent, vector<int>& level, int V) {
    fill(level.begin(), level.end(), -1);  // Reset levels
    queue<int> q;

    level[s] = 0;
    q.push(s);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (int v = 0; v < V; v++) {
            if (level[v] == -1 && rGraph[u][v] > 0) { // If not visited and has capacity
                level[v] = level[u] + 1;  // Assign level
                parent[v] = u;
                q.push(v);

                if (v == t) return true;  // Stop early if we reach the sink
            }
        }
    }
    return false; // No augmenting path found
}

// Ford-Fulkerson Algorithm with Improved BFS
int fordFulkerson(vector<vector<int>>& graph, int s, int t, int V) {
    vector<vector<int>> rGraph = graph;  // Residual graph
    vector<int> parent(V);
    vector<int> level(V);
    int max_flow = 0;

    // Augment the flow while there is a valid path
    while (improvedBFS(rGraph, s, t, parent, level, V)) {
        int path_flow = INT_MAX;

        // Find minimum residual capacity in the path
        for (int v = t; v != s; v = parent[v])
            path_flow = min(path_flow, rGraph[parent[v]][v]);

        // Update residual capacities
        for (int v = t; v != s; v = parent[v]) {
            rGraph[parent[v]][v] -= path_flow;
            rGraph[v][parent[v]] += path_flow;
        }

        max_flow += path_flow;
    }

    return max_flow;
}

// Driver function
int main() {
    int V = 1000;  // Number of vertices
    int edgeDensity = 1000; // Edge density (0-1000) - adjust for sparser or denser graphs

    vector<vector<int>> graph(V, vector<int>(V, 0)); // Initialize graph
    generateRandomGraph(graph, V, edgeDensity); // Generate a random graph

    int source = 0, sink = V - 1;
    
    cout << "The maximum possible flow is: " << fordFulkerson(graph, source, sink, V) << endl;

    return 0;
}
