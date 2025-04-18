#include "mkm.h"

MKM::MKM(int n) : n(n), graph(n) {}

void MKM::addEdge(int u, int v, long long cap) {
    graph[u].push_back({v, (int)graph[v].size(), cap, 0});
    graph[v].push_back({u, (int)graph[u].size() - 1, 0, 0});
}

bool MKM::bfs(int s, int t) {
    level.assign(n, -1);
    queue<int> q;
    level[s] = 0;
    q.push(s);

    while (!q.empty()) {
        int v = q.front(); q.pop();
        for (const Edge &e : graph[v]) {
            if (level[e.to] < 0 && e.flow < e.cap) {
                level[e.to] = level[v] + 1;
                q.push(e.to);
            }
        }
    }

    return level[t] >= 0;
}

long long MKM::blockingFlow(int s, int t) {
    inflow.assign(n, 0);
    outflow.assign(n, 0);

    for (int v = 0; v < n; ++v) {
        if (level[v] == -1) continue;
        for (const Edge &e : graph[v]) {
            if (level[e.to] == level[v] + 1) {
                outflow[v] += e.cap - e.flow;
                inflow[e.to] += e.cap - e.flow;
            }
        }
    }

    queue<int> q;
    vector<bool> dead(n, false);

    for (int i = 0; i < n; ++i)
        if (level[i] != -1 && i != s && i != t)
            q.push(i);

    long long flow = 0;

    while (!q.empty()) {
        int v = q.front(); q.pop();
        if (dead[v]) continue;

        long long pushable = min(inflow[v], outflow[v]);
        if (pushable == 0) continue;

        long long pushed = pushFlow(v, t, pushable);
        flow += pushed;

        inflow[v] -= pushed;
        outflow[v] -= pushed;

        if (inflow[v] == 0 || outflow[v] == 0)
            dead[v] = true;
    }

    return flow;
}

long long MKM::pushFlow(int v, int t, long long flow) {
    if (v == t) return flow;

    for (Edge &e : graph[v]) {
        if (level[e.to] == level[v] + 1 && e.flow < e.cap) {
            long long pushed = pushFlow(e.to, t, min(flow, e.cap - e.flow));
            if (pushed > 0) {
                e.flow += pushed;
                graph[e.to][e.rev].flow -= pushed;
                return pushed;
            }
        }
    }

    return 0;
}

long long MKM::maxFlow(int s, int t) {
    long long flow = 0;
    while (bfs(s, t)) {
        flow += blockingFlow(s, t);
    }
    return flow;
}
