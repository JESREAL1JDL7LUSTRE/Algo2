#ifndef PUSH_RELABEL_H
#define PUSH_RELABEL_H

#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <climits>

using namespace std;

class PushRelabel {
    struct Edge {
        int to, rev;
        long long cap, flow;
    };

    int n;
    vector<vector<Edge>> graph;
    vector<long long> excess;
    vector<int> height, count, active;
    queue<int> Q;

    void push(Edge &e, int u);
    void relabel(int u);
    void discharge(int u);

public:
    explicit PushRelabel(int n);

    void addEdge(int u, int v, long long cap);

    long long maxFlow(int s, int t);
};

#endif // PUSH_RELABEL_H
