#include "push_relabel.h"
#include <queue>
#include <algorithm>

GenericPushRelabel::GenericPushRelabel(int n)
    : n(n),
      adj(n),
      excess(n, 0),
      height(n, 0),
      ptr(n, 0),
      active(n, false)
{}

void GenericPushRelabel::addEdge(int u, int v, long long cap) {
    adj[u].push_back({v, (int)adj[v].size(), cap, 0});
    adj[v].push_back({u, (int)adj[u].size() - 1, 0, 0});
}

long long GenericPushRelabel::maxFlow(int s, int t) {
    // Reset state
    fill(excess.begin(), excess.end(), 0);
    fill(height.begin(), height.end(), 0);
    fill(ptr.begin(), ptr.end(), 0);
    fill(active.begin(), active.end(), false);

    // Initial preflow from source
    height[s] = n;
    for (auto &e : adj[s]) {
        e.flow = e.cap;
        adj[e.to][e.rev].flow = -e.cap;
        excess[e.to] += e.cap;
        excess[s] -= e.cap;
    }

    // Enqueue all active vertices except s & t
    queue<int> q;
    for (int i = 0; i < n; i++) {
        if (i != s && i != t && excess[i] > 0) {
            q.push(i);
            active[i] = true;
        }
    }

    // Main discharge loop
    while (!q.empty()) {
        int u = q.front(); q.pop();
        active[u] = false;

        // Discharge u until its excess is 0
        while (excess[u] > 0) {
            if (ptr[u] == (int)adj[u].size()) {
                // Relabel
                int min_h = INT_MAX;
                for (auto &e : adj[u]) {
                    if (e.cap - e.flow > 0)
                        min_h = min(min_h, height[e.to]);
                }
                height[u] = (min_h < INT_MAX ? min_h + 1 : height[u]);
                ptr[u] = 0;
            } else {
                // Try to push
                auto &e = adj[u][ptr[u]];
                if (e.cap - e.flow > 0 && height[u] == height[e.to] + 1) {
                    long long delta = min(excess[u], e.cap - e.flow);
                    e.flow += delta;
                    adj[e.to][e.rev].flow -= delta;
                    excess[u] -= delta;
                    excess[e.to] += delta;

                    if (!active[e.to] && e.to != s && e.to != t) {
                        q.push(e.to);
                        active[e.to] = true;
                    }
                } else {
                    ptr[u]++;
                }
            }
        }
    }

    // Sum flows out of s
    long long flow = 0;
    for (auto &e : adj[s]) flow += e.flow;
    return flow;
}
