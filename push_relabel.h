#ifndef GENERIC_PUSH_RELABEL_H
#define GENERIC_PUSH_RELABEL_H

#include <vector>
#include <climits>
using namespace std;

class GenericPushRelabel {
    struct Edge {
        int to, rev;
        long long cap, flow;
    };

    int n;
    vector<vector<Edge>> adj;
    vector<long long> excess;
    vector<int> height, ptr;
    vector<bool> active;

public:
    explicit GenericPushRelabel(int n);
    void addEdge(int u, int v, long long cap);
    long long maxFlow(int s, int t);
};

#endif // GENERIC_PUSH_RELABEL_H
