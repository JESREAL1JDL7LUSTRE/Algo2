#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "harernasAlgo.h"

using namespace std;

    Dinic::Dinic(int V) : V(V), adj(V), level(V, -1), ptr(V, 0) {}

    // Add an edge from u to v with given capacity, and the reverse edge with 0 capacity.
    void Dinic::addEdge(int u, int v, int cap) {
        adj[u].push_back({v, 0, cap, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    // ---------------- Parallel BFS Worker (as described in the paper) ----------------
    // This worker processes a chunk of the frontier and writes discovered nodes into its local buffer.
    void Dinic::bfs_worker(const vector<int>& frontier, int start, int end,
                    vector<vector<int>>& local_frontiers, int thread_id) {
        for (int i = start; i < end; i++) {
            int u = frontier[i];
            for (auto &e : adj[u]) {
                // Use lock_guard to update shared 'level'
                lock_guard<mutex> lock(level_mutex);
                if (level[e.v] == -1 && e.flow < e.cap) {
                    level[e.v] = level[u] + 1;
                    local_frontiers[thread_id].push_back(e.v);
                }
            }
        }
    }

    // ---------------- Parallel BFS ----------------
    // This function partitions the current frontier among threads, each writes into its local buffer.
    bool Dinic::parallelBFS(int s, int t) {
        fill(level.begin(), level.end(), -1);
        vector<int> frontier;
        frontier.push_back(s);
        level[s] = 0;

        // Local buffers for each thread.
        vector<vector<int>> local_frontiers(NUM_THREADS);
        for (int i = 0; i < NUM_THREADS; i++) {
            local_frontiers[i].reserve(V / NUM_THREADS + 10);
        }
        
        while (!frontier.empty()) {
            int f_size = frontier.size();
            // Use fewer threads if frontier is small.
            int num_threads = min(NUM_THREADS, max(1, f_size / 500));
            
            for (auto &lf : local_frontiers)
                lf.clear();
            
            vector<thread> threads;
            int chunk_size = (f_size + num_threads - 1) / num_threads;
            for (int i = 0; i < num_threads; i++) {
                int start = i * chunk_size;
                int end = min((i + 1) * chunk_size, f_size);
                if (start < end) {
                    threads.emplace_back(&Dinic::bfs_worker, this, cref(frontier), start, end, ref(local_frontiers), i);
                }
            }
            for (auto &th : threads)
                th.join();
            
            vector<int> next_frontier;
            next_frontier.reserve(f_size);
            for (auto &lf : local_frontiers)
                next_frontier.insert(next_frontier.end(), lf.begin(), lf.end());
            
            frontier.swap(next_frontier);
        }
        return level[t] != -1;
    }

    // ---------------- Sequential DFS (unchanged) ----------------
    int Dinic::dfs(int u, int t, int flow) {
        if (u == t)
            return flow;
        for (int &i = ptr[u]; i < adj[u].size(); i++) {
            auto &e = adj[u][i];
            if (level[e.v] == level[u] + 1 && e.flow < e.cap) {
                int pushed = dfs(e.v, t, min(flow, e.cap - e.flow));
                if (pushed > 0) {
                    e.flow += pushed;
                    adj[e.v][e.rev].flow -= pushed;
                    return pushed;
                }
            }
        }
        return 0;
    }

    // ---------------- Max Flow Computation ----------------
    int Dinic::maxFlow(int s, int t) {
        int flow = 0;
        while (parallelBFS(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (int pushed = dfs(s, t, INF))
                flow += pushed;
        }
        return flow;
    }