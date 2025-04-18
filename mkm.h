#ifndef MKM_H
#define MKM_H

#include <vector>
#include <queue>
#include <limits>
#include <algorithm>

using namespace std;

class MKM {
    struct Edge {
        int to, rev;
        long long cap, flow;
    };

    int n;
    vector<vector<Edge>> graph;
    vector<int> level;
    vector<long long> inflow, outflow;

    bool bfs(int s, int t);
    long long blockingFlow(int s, int t);
    long long pushFlow(int v, int t, long long flow);

public:
    explicit MKM(int n);

    void addEdge(int u, int v, long long cap);
    long long maxFlow(int s, int t);
};

#endif // MKM_H
