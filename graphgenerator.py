import json
import random

def generate_graph(num_nodes, num_edges, max_capacity=100, output_file="graph.json"):
    graph = {
        "nodes": list(range(num_nodes)),
        "edges": []
    }

    edge_set = set()

    def add_edge(u, v, cap):
        # 1) no self‑loops, 2) no parallel in either direction
        if u == v or (u, v) in edge_set or (v, u) in edge_set:
            return False

        edge_set.add((u, v))
        graph["edges"].append({
            "from": u,
            "to": v,
            "capacity": cap
        })
        return True

    # 1) Guarantee connectivity along a spine 0→1→…→N-1
    for i in range(num_nodes - 1):
        add_edge(i, i + 1, random.randint(30, max_capacity))

    # 2) Fill up remaining edges
    while len(graph["edges"]) < num_edges:
        u = random.randint(0, num_nodes - 1)
        v = random.randint(0, num_nodes - 1)
        cap = random.randint(30, max_capacity)
        add_edge(u, v, cap)

    with open(output_file, "w") as f:
        json.dump(graph, f)

    print(f"Graph with {num_nodes} nodes and {len(graph['edges'])} edges saved to {output_file}")

# Example usage
generate_graph(num_nodes=700000, num_edges=1500000, max_capacity=100,
               output_file="testgraph700k1.5M.json")
