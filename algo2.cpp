
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <iostream>
#include <queue>
#include <climits>
#include <cstring>

using namespace std;
const int INF = numeric_limits<int>::max();
#define V 6 

int widestPath(int s, int t, const vector<vector<int>> &res, vector<int>& parent) {
    int n = res.size();
    vector<int> bottleneck(n, 0);

    priority_queue<pair<int,int>> pq;
    bottleneck[s] = INF;
    parent.assign(n, -1);
    pq.push({bottleneck[s], s});

    while(!pq.empty()){
        int curFlow = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if(u == t)
            return bottleneck[t];

        for (int v = 0; v < n; v++){
            if (res[u][v] > 0) {
                int newFlow = min(curFlow, res[u][v]);
                if(newFlow > bottleneck[v]) {
                    bottleneck[v] = newFlow;
                    parent[v] = u;
                    pq.push({bottleneck[v], v});
                }
            }
        }
    }
    return bottleneck[t];
}

int someAlgotithm(const vector<vector<int>> &capacity, int s, int t) {
    int n = capacity.size();
    int maxFlow = 0;
    vector<vector<int>> res = capacity;
    vector<int> parent(n);




}

int main() {
    vector<vector<int>> capacity = {
        {0, 16, 13, 0, 0, 0},
        {0, 0, 10, 12, 0, 0},
        {0, 4, 0, 0, 14, 0},
        {0, 0, 9, 0, 0, 20},
        {0, 0, 0, 7, 0, 4},
        {0, 0, 0, 0, 0, 0}
    };

    int source = 0;
    int sink = 5;

    int maxFlow = someAlgotithm(capacity, source, sink);
    cout << "The maximum possible flow is " << maxFlow << endl;
    return 0;
}