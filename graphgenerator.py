import json
import random

def generate_graph(num_nodes, num_edges, max_capacity=100):
    graph = {
        "nodes": list(range(num_nodes)),
        "edges": []
    }

    # Create an edge from node 0 to node num_nodes-1 to guarantee a path
    graph["edges"].append({
        "from": 0, 
        "to": num_nodes - 1, 
        "capacity": random.randint(30, max_capacity)
    })

    # Now create the rest of the graph with random edges
    for _ in range(num_edges - 1):
        u = random.randint(0, num_nodes - 1)
        v = random.randint(0, num_nodes - 1)
        while u == v:  # avoid self-loops
            v = random.randint(0, num_nodes - 1)
        cap = random.randint(30, max_capacity)
        graph["edges"].append({"from": u, "to": v, "capacity": cap})

    with open("graph.json", "w") as f:
        json.dump(graph, f)

generate_graph(num_nodes=100000, num_edges=500000)
