#include "push_relabel.h"

PushRelabel::PushRelabel(int n)
    : n(n), graph(n), excess(n), height(n), count(2 * n), active(n) {}

void PushRelabel::addEdge(int u, int v, long long cap) {
    graph[u].push_back({v, (int)graph[v].size(), cap, 0});
    graph[v].push_back({u, (int)graph[u].size() - 1, 0, 0});
}

void PushRelabel::push(Edge &e, int u) {
    long long amt = min(excess[u], e.cap - e.flow);
    if (height[u] == height[e.to] + 1 && amt > 0) {
        e.flow += amt;
        graph[e.to][e.rev].flow -= amt;
        excess[e.to] += amt;
        excess[u] -= amt;
        if (!active[e.to] && excess[e.to] > 0) {
            active[e.to] = 1;
            Q.push(e.to);
        }
    }
}

void PushRelabel::relabel(int u) {
    int min_height = INT_MAX;
    for (auto &e : graph[u]) {
        if (e.cap > e.flow)
            min_height = min(min_height, height[e.to]);
    }
    if (min_height < INT_MAX) {
        count[height[u]]--;
        height[u] = min_height + 1;
        count[height[u]]++;
    }
}

void PushRelabel::discharge(int u) {
    while (excess[u] > 0) {
        for (auto &e : graph[u]) {
            push(e, u);
            if (excess[u] == 0) break;
        }
        if (excess[u] > 0) {
            relabel(u);
        }
    }
}

long long PushRelabel::maxFlow(int s, int t) {
    height[s] = n;
    count[0] = n - 1;
    count[n] = 1;

    for (auto &e : graph[s]) {
        excess[s] -= e.cap;
        excess[e.to] += e.cap;
        e.flow = e.cap;
        graph[e.to][e.rev].flow = -e.cap;
        if (e.to != s && e.to != t) {
            active[e.to] = 1;
            Q.push(e.to);
        }
    }

    while (!Q.empty()) {
        int u = Q.front(); Q.pop();
        active[u] = 0;
        discharge(u);
    }

    long long flow = 0;
    for (auto &e : graph[s])
        flow += e.flow;
    return flow;
}
