#pragma once
#include <unordered_map>
#include <vector>
#include <string>

struct Edge {
    int to;
    int capacity;
};

using Graph = std::unordered_map<int, std::vector<Edge>>;

bool load_graph_from_json(const std::string& filename, Graph& graph, std::unordered_map<int, int>& id_to_index);
