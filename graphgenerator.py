
import json
import random

def generate_strongly_connected_graph(num_nodes, num_edges, max_capacity=100, output_file="graph.json"):
    graph = {
        "nodes": list(range(num_nodes)),
        "edges": []
    }
    edge_set = set()

    def add_edge(u, v, cap):
        if u == v or (u, v) in edge_set or (v, u) in edge_set:
            return False
        edge_set.add((u, v))
        graph["edges"].append({
            "from": u,
            "to": v,
            "capacity": cap
        })
        return True

    # 1. Create layered structure
    layer_count = max(2, int(num_nodes ** 0.5))  # sqrt(N) layers
    layers = [[] for _ in range(layer_count)]

    # First pass: assign each node based on index
    for i in range(num_nodes):
        layer_idx = int((i / num_nodes) * (layer_count - 1))
        layers[layer_idx].append(i)

    # Second pass: ensure no layer is empty
    for i in range(layer_count):
        if not layers[i]:
            # Move a random node into this empty layer
            donor_layer = random.choice([j for j in range(layer_count) if len(layers[j]) > 1])
            moved_node = layers[donor_layer].pop()
            layers[i].append(moved_node)

    # 2. Connect each layer to the next
    for i in range(layer_count - 1):
        for u in layers[i]:
            v = random.choice(layers[i + 1])
            cap = random.randint(20, max_capacity)
            add_edge(u, v, cap)

    # 3. Add random edges
    while len(graph["edges"]) < num_edges:
        u = random.randint(0, num_nodes - 1)
        v = random.randint(0, num_nodes - 1)
        cap = random.randint(10, max_capacity)
        add_edge(u, v, cap)

    # 4. Strong s→t chain
    for i in range(num_nodes - 1):
        add_edge(i, i + 1, random.randint(80, max_capacity))

    # 5. Save graph
    with open(output_file, "w") as f:
        json.dump(graph, f)

    print(f"Generated graph with {num_nodes} nodes and {len(graph['edges'])} edges saved to '{output_file}'")

# Usage example
generate_strongly_connected_graph(num_nodes=10, num_edges=30, output_file="SG.json")