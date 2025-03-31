#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

using namespace std;

struct Edge {
    int v, capacity, flow;
};

class Graph {
public:
    int V;
    vector<vector<Edge>> adj;
    vector<int> level;

    Graph(int V) : V(V), adj(V), level(V, -1) {}

    void addEdge(int u, int v, int capacity) {
        adj[u].push_back({v, capacity, 0});
        adj[v].push_back({u, 0, 0}); // Reverse edge with 0 capacity
    }
};

queue<int> frontier;
mutex frontier_mutex;
condition_variable cv;
atomic<bool> work_done(false);

void bfs_worker(Graph &graph) {
    while (!work_done) {
        int u;
        {
            unique_lock<mutex> lock(frontier_mutex);
            if (frontier.empty()) return; // No work left
            u = frontier.front();
            frontier.pop();
        }

        for (auto &edge : graph.adj[u]) {
            if (graph.level[edge.v] == -1 && edge.flow < edge.capacity) {
                {
                    lock_guard<mutex> lock(frontier_mutex);
                    graph.level[edge.v] = graph.level[u] + 1;
                    frontier.push(edge.v);
                }
                cv.notify_all();
            }
        }
    }
}

bool parallelBFS(Graph &graph, int s, int t, int num_threads) {
    fill(graph.level.begin(), graph.level.end(), -1);
    {
        lock_guard<mutex> lock(frontier_mutex);
        graph.level[s] = 0;
        frontier.push(s);
    }

    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(bfs_worker, ref(graph));
    }

    for (auto &t : threads) t.join();

    return graph.level[t] != -1;
}

int main() {
    int V = 1e6 + 1;  // 🔥 Large graph with 1M nodes
    Graph graph(V);

    cout << "Number of nodes: " << V << endl;

    // 🔥 Create a large random graph
    for (int i = 0; i < V - 1; i++) {
        graph.addEdge(i, i + 1, rand() % 50 + 1);
        if (i + 2 < V) graph.addEdge(i, i + 2, rand() % 50 + 1);
    }

    int num_threads = thread::hardware_concurrency(); // Get max available cores
    cout << "Using " << num_threads << " threads for BFS." << endl;

    bool bfs_result = parallelBFS(graph, 0, 5, num_threads);

    cout << "BFS Result: " << (bfs_result ? "Path Found" : "No Path") << endl;

    return 0;
}
