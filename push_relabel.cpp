#include "push_relabel.h"
#include <queue>
#include <cstring>

OptimizedPushRelabel::OptimizedPushRelabel(int n)
    : n(n), graph(n), excess(n), height(n), count(2 * n), active(n) {}

void OptimizedPushRelabel::addEdge(int u, int v, long long cap) {
    graph[u].push_back({v, (int)graph[v].size(), cap, 0});
    graph[v].push_back({u, (int)graph[u].size() - 1, 0, 0});
}

void OptimizedPushRelabel::push(int u, Edge &e) {
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

void OptimizedPushRelabel::relabel(int u) {
    int min_height = INT_MAX;
    for (auto &e : graph[u]) {
        if (e.cap > e.flow) {
            min_height = min(min_height, height[e.to]);
        }
    }
    if (min_height < INT_MAX) {
        count[height[u]]--;
        height[u] = min_height + 1;
        count[height[u]]++;
    }
}

void OptimizedPushRelabel::gap(int k) {
    for (int u = 0; u < n; ++u) {
        if (height[u] >= k) {
            count[height[u]]--;
            height[u] = max(height[u], n + 1);
            count[height[u]]++;
        }
    }
}

void OptimizedPushRelabel::discharge(int u) {
    for (auto &e : graph[u]) {
        if (excess[u] == 0) break;
        push(u, e);
    }
    if (excess[u] > 0) {
        if (count[height[u]] == 1) {
            gap(height[u]);
        } else {
            relabel(u);
        }
    }
}

void OptimizedPushRelabel::global_relabel(int t) {
    fill(height.begin(), height.end(), n);
    fill(count.begin(), count.end(), 0);

    queue<int> q;
    height[t] = 0;
    q.push(t);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (auto &e : graph[u]) {
            if (graph[e.to][e.rev].cap > graph[e.to][e.rev].flow && height[e.to] == n) {
                height[e.to] = height[u] + 1;
                q.push(e.to);
            }
        }
        count[height[u]]++;
    }
}

long long OptimizedPushRelabel::maxFlow(int s, int t) {
    global_relabel(t);

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

    int discharge_counter = 0;
    const int RELABEL_INTERVAL = n;

    while (!Q.empty()) {
        int u = Q.front(); Q.pop();
        active[u] = 0;
        discharge(u);

        if (++discharge_counter % RELABEL_INTERVAL == 0)
            global_relabel(t);

        if (excess[u] > 0) {
            active[u] = 1;
            Q.push(u);
        }
    }

    long long flow = 0;
    for (auto &e : graph[s])
        flow += e.flow;
    return flow;
}
