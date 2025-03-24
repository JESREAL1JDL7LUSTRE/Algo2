Algorithm Ford-Fulkerson(graph, source, sink)
    Initialize max_flow = 0
    Create residual graph rGraph, initially same as graph

    While there exists an augmenting path from source to sink in rGraph using BFS:
        Find the minimum residual capacity (bottleneck) along the path
        For each edge in the path:
            Reduce the residual capacity of the edge by bottleneck value
            Increase the residual capacity of the reverse edge by bottleneck value
        Add bottleneck value to max_flow

    Return max_flow

Function BFS(rGraph, source, sink, parent)
    Initialize visited array with False
    Create queue and enqueue source
    Mark source as visited
    Set parent of source to -1

    While queue is not empty:
        Dequeue vertex u
        For each adjacent vertex v:
            If v is not visited and residual capacity from u to v is greater than 0:
                If v is the sink, update parent[v] and return True
                Enqueue v
                Update parent[v] to u
                Mark v as visited

    Return False (if no augmenting path is found)

Main Program
    Define graph as adjacency matrix
    Call Ford-Fulkerson(graph, source, sink)
    Print max flow
