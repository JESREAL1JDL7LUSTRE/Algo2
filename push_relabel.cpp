// #include "push_relabel.h"
// #include <queue>
// #include <algorithm>

// GenericPushRelabel::GenericPushRelabel(int n)
//     : n(n),
//       adj(n),
//       excess(n, 0),
//       height(n, 0),
//       ptr(n, 0),
//       active(n, false)
// {}

// void GenericPushRelabel::addEdge(int u, int v, long long cap) {
//     adj[u].push_back({v, (int)adj[v].size(), cap, 0});
//     adj[v].push_back({u, (int)adj[u].size() - 1, 0, 0});
// }

// long long GenericPushRelabel::maxFlow(int s, int t) {
//     // Reset state
//     fill(excess.begin(), excess.end(), 0);
//     fill(height.begin(), height.end(), 0);
//     fill(ptr.begin(), ptr.end(), 0);
//     fill(active.begin(), active.end(), false);

//     // Initial preflow from source
//     height[s] = n;
//     for (auto &e : adj[s]) {
//         e.flow = e.cap;
//         adj[e.to][e.rev].flow = -e.cap;
//         excess[e.to] += e.cap;
//         excess[s] -= e.cap;
//     }

//     // Enqueue all active vertices except s & t
//     queue<int> q;
//     for (int i = 0; i < n; i++) {
//         if (i != s && i != t && excess[i] > 0) {
//             q.push(i);
//             active[i] = true;
//         }
//     }

//     // Main discharge loop
//     while (!q.empty()) {
//         int u = q.front(); q.pop();
//         active[u] = false;

//         // Discharge u until its excess is 0
//         while (excess[u] > 0) {
//             if (ptr[u] == (int)adj[u].size()) {
//                 // Relabel
//                 int min_h = INT_MAX;
//                 for (auto &e : adj[u]) {
//                     if (e.cap - e.flow > 0)
//                         min_h = min(min_h, height[e.to]);
//                 }
//                 height[u] = (min_h < INT_MAX ? min_h + 1 : height[u]);
//                 ptr[u] = 0;
//             } else {
//                 // Try to push
//                 auto &e = adj[u][ptr[u]];
//                 if (e.cap - e.flow > 0 && height[u] == height[e.to] + 1) {
//                     long long delta = min(excess[u], e.cap - e.flow);
//                     e.flow += delta;
//                     adj[e.to][e.rev].flow -= delta;
//                     excess[u] -= delta;
//                     excess[e.to] += delta;

//                     if (!active[e.to] && e.to != s && e.to != t) {
//                         q.push(e.to);
//                         active[e.to] = true;
//                     }
//                 } else {
//                     ptr[u]++;
//                 }
//             }
//         }
//     }

//     // Sum flows out of s
//     long long flow = 0;
//     for (auto &e : adj[s]) flow += e.flow;
//     return flow;
// }


#include "push_relabel.h"
#include <queue>
#include <algorithm>
#include <limits>

ImprovedPushRelabel::ImprovedPushRelabel(int n)
    : n(n),
      adj(n),
      excess(n, 0),
      height(n, 0),
      count(2*n, 0),
      current(n, 0),
      active(n, false),
      relabel_counter(0),
      max_height(0)
{}

void ImprovedPushRelabel::addEdge(int u, int v, long long cap) {
    adj[u].push_back({v, (int)adj[v].size(), cap, 0});
    adj[v].push_back({u, (int)adj[u].size() - 1, 0, 0});
}

void ImprovedPushRelabel::push(int u, Edge& e) {
    long long delta = std::min(excess[u], e.cap - e.flow);
    if (delta <= 0) return;
    
    e.flow += delta;
    adj[e.to][e.rev].flow -= delta;
    excess[u] -= delta;
    excess[e.to] += delta;
    
    // Activate the destination if it's not a source or sink
    if (!active[e.to] && e.to != 0 && e.to != n-1 && excess[e.to] > 0) {
        active[e.to] = true;
        excess_vertices.push_back(e.to);
    }
}

void ImprovedPushRelabel::relabel(int u) {
    // Reduce count of vertices at current height
    count[height[u]]--;
    
    // Find the minimum height of a neighbor through which we can push
    int min_height = 2*n;
    for (auto& e : adj[u]) {
        if (e.cap > e.flow) {
            min_height = std::min(min_height, height[e.to]);
        }
    }
    
    // New height is one more than the minimum height
    height[u] = min_height + 1;
    
    // Update the maximum height
    max_height = std::max(max_height, height[u]);
    
    // Increase count of vertices at new height
    count[height[u]]++;
    
    // Reset the current edge
    current[u] = 0;
    
    // Count relabels for global relabeling heuristic
    relabel_counter++;
}

void ImprovedPushRelabel::gap(int h) {
    // Gap heuristic: If there are no vertices at height h, 
    // then all vertices with height > h can't reach the sink
    for (int i = 0; i < n; i++) {
        if (height[i] >= h && height[i] < n) {
            // Update counts and set unreachable vertices to maximum height
            count[height[i]]--;
            height[i] = std::max(height[i], n);
            count[height[i]]++;
            
            // Reset current edge
            current[i] = 0;
        }
    }
}

void ImprovedPushRelabel::global_relabel(int t) {
    // Reset heights to maximum (unreachable)
    std::fill(height.begin(), height.end(), n);
    std::fill(count.begin(), count.end(), 0);
    
    // BFS from sink to recompute heights as distance from sink
    height[t] = 0;
    std::queue<int> q;
    q.push(t);
    
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        
        for (auto& e : adj[u]) {
            // Look at residual edges (backward edges with capacity)
            if (adj[e.to][e.rev].cap > adj[e.to][e.rev].flow && height[e.to] == n) {
                height[e.to] = height[u] + 1;
                count[height[e.to]]++;
                q.push(e.to);
            }
        }
    }
    
    // Reset current edges
    std::fill(current.begin(), current.end(), 0);
    
    // Reset active vertices based on new heights
    excess_vertices.clear();
    for (int i = 0; i < n; i++) {
        if (i != 0 && i != t && excess[i] > 0 && height[i] < n) {
            active[i] = true;
            excess_vertices.push_back(i);
        } else {
            active[i] = false;
        }
    }
    
    // Compute max height
    max_height = 0;
    for (int i = 0; i < n; i++) {
        if (height[i] < n) {
            max_height = std::max(max_height, height[i]);
        }
    }
    
    // Reset counter
    relabel_counter = 0;
}

void ImprovedPushRelabel::discharge(int u) {
    while (excess[u] > 0) {
        // If we've tried all edges, relabel
        if (current[u] >= (int)adj[u].size()) {
            relabel(u);
            
            // Check if we need to do global relabeling
            if (relabel_counter >= GLOBAL_RELABEL_FREQ * n) {
                global_relabel(n-1); // Assuming sink is n-1
            }
            
            // If we reached max height after relabeling, stop discharging
            if (height[u] >= n) {
                break;
            }
        } else {
            // Try to push through the current edge
            Edge& e = adj[u][current[u]];
            if (e.cap > e.flow && height[u] == height[e.to] + 1) {
                push(u, e);
            } else {
                current[u]++;
            }
        }
    }
}

long long ImprovedPushRelabel::maxFlow(int s, int t) {
    // Reset state
    std::fill(excess.begin(), excess.end(), 0);
    std::fill(height.begin(), height.end(), 0);
    std::fill(count.begin(), count.end(), 0);
    std::fill(current.begin(), current.end(), 0);
    std::fill(active.begin(), active.end(), false);
    excess_vertices.clear();
    relabel_counter = 0;
    max_height = 0;
    
    // Initial preflow from source
    height[s] = n;
    count[n]++;
    
    for (auto& e : adj[s]) {
        e.flow = e.cap;
        adj[e.to][e.rev].flow = -e.cap;
        excess[e.to] += e.cap;
        
        // Add to active vertices if not source or sink
        if (e.to != s && e.to != t && !active[e.to] && excess[e.to] > 0) {
            active[e.to] = true;
            excess_vertices.push_back(e.to);
        }
    }
    
    // Initial global relabeling
    global_relabel(t);
    
    // Main discharge loop - Highest label selection
    while (!excess_vertices.empty()) {
        // Find vertex with highest height
        int idx = -1;
        int max_h = -1;
        
        for (int i = 0; i < (int)excess_vertices.size(); i++) {
            int v = excess_vertices[i];
            if (active[v] && height[v] > max_h) {
                max_h = height[v];
                idx = i;
            }
        }
        
        if (idx == -1) break;  // No active vertices
        
        int u = excess_vertices[idx];
        
        // Remove from excess_vertices by swapping with last
        std::swap(excess_vertices[idx], excess_vertices.back());
        excess_vertices.pop_back();
        active[u] = false;
        
        // Discharge vertex
        discharge(u);
        
        // Check for gap
        for (int h = 1; h <= max_height; h++) {
            if (count[h] == 0) {
                gap(h);
                break;
            }
        }
    }
    
    // Return the flow (excess at sink)
    return excess[t];
}

std::vector<bool> ImprovedPushRelabel::getMinCut(int s) {
    std::vector<bool> reachable(n, false);
    std::queue<int> q;
    q.push(s);
    reachable[s] = true;
    
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        
        for (auto& e : adj[u]) {
            if (e.cap > e.flow && !reachable[e.to]) {
                reachable[e.to] = true;
                q.push(e.to);
            }
        }
    }
    
    return reachable;
}