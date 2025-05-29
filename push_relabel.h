
#ifndef IMPROVED_PUSH_RELABEL_H
#define IMPROVED_PUSH_RELABEL_H

#include <vector>
#include <limits>

class ImprovedPushRelabel {
private:
    struct Edge {
        int to;
        int rev;
        long long cap;
        long long flow;
    };

    int n;                      // Number of vertices
    std::vector<std::vector<Edge>> adj;  // Adjacency list
    std::vector<long long> excess;  // Excess flow at each vertex
    std::vector<int> height;    // Height function
    std::vector<int> count;     // Count of vertices at each height
    std::vector<int> current;   // Current edge for each vertex
    std::vector<bool> active;   // Whether a vertex is active
    std::vector<int> excess_vertices; // List of vertices with excess flow

    static const int GLOBAL_RELABEL_FREQ = 2; // Frequency of global relabeling
    int relabel_counter;        // Counter for global relabeling
    int max_height;             // Maximum height of any vertex

    void push(int u, Edge& e);
    void relabel(int u);
    void discharge(int u);
    void gap(int h);
    void global_relabel(int t);
    
public:
    ImprovedPushRelabel(int n);
    void addEdge(int u, int v, long long cap);
    long long maxFlow(int s, int t);
    // Additional function to extract min cut
    std::vector<bool> getMinCut(int s);
};

#endif // IMPROVED_PUSH_RELABEL_H