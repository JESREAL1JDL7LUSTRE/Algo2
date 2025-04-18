#ifndef CAPACITY_SCALING_H
#define CAPACITY_SCALING_H

#include <vector>
#include <limits>
#include <algorithm>

using namespace std;

class CapacityScaling {
    struct Edge {
        int to, rev;
        long long cap;
    };

    int n;
    vector<vector<Edge>> graph;
    vector<bool> visited;

    long long dfs(int v, int t, long long f, long long min_cap);

public:
    explicit CapacityScaling(int n);

    void addEdge(int u, int v, long long cap);

    long long maxFlow(int s, int t);
};

#endif // CAPACITY_SCALING_H
