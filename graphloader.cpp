#include "graphloader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool load_graph_from_json(const std::string& filename, Graph& graph) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return false;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    const auto& nodes = j["nodes"];
    const auto& edges = j["edges"];

    for (const auto& node : nodes) {
        graph[node] = {};  // Initialize empty edge list for each node
    }

    for (const auto& edge : edges) {
        int u = edge["from"];
        int v = edge["to"];
        int cap = edge["capacity"];
        graph[u].push_back({v, cap});
    }

    return true;
}
