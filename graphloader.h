#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct Edge {
    int to;
    int capacity;
};

using Graph = std::unordered_map<int, std::vector<Edge>>;

bool load_graph_from_json(const std::string& filename, Graph& graph);
