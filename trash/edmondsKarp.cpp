#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <cstdlib>  // For rand()
#include <ctime>    // For time()
#include <omp.h>    // OpenMP for parallel BFS

using namespace std;

// Function to generate a random graph with adjacency list
void generateRandomGraph(vector<vector<pair<int, int>>>& graph, int V, int edgeDensity) {
    srand(time(0)); // Seed for randomness

    for (int i = 0; i < V; i++) {
        for (int j = 0; j < V; j++) {
            if (i != j && (rand() % 1000 < edgeDensity)) {  // Control edge density (probability-based)
                int weight = rand() % 41 + 10;  // Random weight between 10-50
                graph[i].push_back({j, weight});  // Directed edge i → j
            }
        }
    }
}

// Parallel BFS that constructs a level graph using OpenMP
bool parallelBFS(vector<vector<pair<int, int>>>& rGraph, int s, int t, vector<int>& parent, vector<int>& level, int V) {
    fill(level.begin(), level.end(), -1);  // Reset levels
    queue<int> q;
    level[s] = 0;
    q.push(s);
    
    bool foundPath = false;

    #pragma omp parallel
    {
        queue<int> localQ; // Local queue per thread
        #pragma omp single nowait
        {
            while (!q.empty()) {
                int u = q.front();
                q.pop();

                #pragma omp for nowait
                for (size_t i = 0; i < rGraph[u].size(); i++) {
                    int v = rGraph[u][i].first;
                    int capacity = rGraph[u][i].second;

                    if (level[v] == -1 && capacity > 0) { // If not visited and has capacity
                        level[v] = level[u] + 1;
                        parent[v] = u;

                        if (v == t) {
                            foundPath = true;
                        }

                        localQ.push(v);  // Use thread-local queue
                    }
                }

                // Merge thread-local queues back into main queue
                #pragma omp critical
                while (!localQ.empty()) {
                    q.push(localQ.front());
                    localQ.pop();
                }
            }
        }
    }
    
    return foundPath;
}

// Ford-Fulkerson Algorithm with Parallel BFS
int fordFulkerson(vector<vector<pair<int, int>>>& graph, int s, int t, int V) {
    vector<vector<pair<int, int>>> rGraph = graph;  // Residual graph
    vector<int> parent(V);
    vector<int> level(V);
    int max_flow = 0;

    // Augment the flow while there is a valid path
    while (parallelBFS(rGraph, s, t, parent, level, V)) {
        int path_flow = INT_MAX;

        // Find minimum residual capacity in the path
        for (int v = t; v != s; v = parent[v]) {
            for (auto& edge : rGraph[parent[v]]) {
                if (edge.first == v) {
                    path_flow = min(path_flow, edge.second);
                    break;
                }
            }
        }

        // Update residual capacities
        for (int v = t; v != s; v = parent[v]) {
            for (auto& edge : rGraph[parent[v]]) {
                if (edge.first == v) {
                    edge.second -= path_flow;
                    break;
                }
            }
            for (auto& edge : rGraph[v]) {
                if (edge.first == parent[v]) {
                    edge.second += path_flow;
                    break;
                }
            }
        }

        max_flow += path_flow;
    }

    return max_flow;
}

// Driver function
int main() {
    int V = 1000;  // Number of vertices
    int edgeDensity = 1000; // Edge density (0-1000) - adjust for sparser or denser graphs

    vector<vector<pair<int, int>>> graph(V);  // Adjacency list representation
    generateRandomGraph(graph, V, edgeDensity); // Generate a random graph

    int source = 0, sink = V - 1;
    
    cout << "The maximum possible flow is: " << fordFulkerson(graph, source, sink, V) << endl;

    return 0;
}
