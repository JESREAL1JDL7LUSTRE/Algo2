#ifndef OPTIMIZED_PUSH_RELABEL_H
#define OPTIMIZED_PUSH_RELABEL_H

#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <climits>

using namespace std;

class OptimizedPushRelabel {
    struct Edge {
        int to, rev;
        long long cap, flow;
    };

    int n;
    vector<vector<Edge>> graph;
    vector<long long> excess;
    vector<int> height, count, active;
    queue<int> Q;

    void push(int u, Edge &e);
    void relabel(int u);
    void gap(int k);
    void discharge(int u);
    void global_relabel(int t);

public:
    explicit OptimizedPushRelabel(int n);

    void addEdge(int u, int v, long long cap);
    long long maxFlow(int s, int t);
};

#endif // OPTIMIZED_PUSH_RELABEL_H
