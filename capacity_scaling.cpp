#include "capacity_scaling.h"

CapacityScaling::CapacityScaling(int n): n(n), graph(n), visited(n) {}

void CapacityScaling::addEdge(int u, int v, long long cap) {
    graph[u].push_back({v, (int)graph[v].size(), cap});
    graph[v].push_back({u, (int)graph[u].size() - 1, 0}); // reverse edge
}

long long CapacityScaling::dfs(int v, int t, long long f, long long min_cap) {
    if (v == t) return f;
    visited[v] = true;
    for (auto &e : graph[v]) {
        if (!visited[e.to] && e.cap >= min_cap) {
            long long d = dfs(e.to, t, min(f, e.cap), min_cap);
            if (d > 0) {
                e.cap -= d;
                graph[e.to][e.rev].cap += d;
                return d;
            }
        }
    }
    return 0;
}

long long CapacityScaling::maxFlow(int s, int t) {
    long long flow = 0;
    long long max_cap = 0;

    for (int u = 0; u < n; ++u)
        for (auto &e : graph[u])
            max_cap = max(max_cap, e.cap);

    for (long long scale = 1LL << (63 - __builtin_clzll(max_cap)); scale > 0; scale >>= 1) {
        while (true) {
            fill(visited.begin(), visited.end(), false);
            long long pushed = dfs(s, t, LLONG_MAX, scale);
            if (pushed == 0) break;
            flow += pushed;
        }
    }
    return flow;
}
