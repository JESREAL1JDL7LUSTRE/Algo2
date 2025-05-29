import json
import sys
from pathlib import Path

def parse_txt_to_graph(input_file):
    """
    Parse a text file containing graph data and convert to JSON format.
    
    Expected input format: from_node to_node capacity
    Example: 1 51 1 (node 1 to node 51 with capacity 1)
    """
    nodes = set()
    edges = []
    
    try:
        with open(input_file, 'r') as file:
            for line_num, line in enumerate(file, 1):
                line = line.strip()
                if not line or line.startswith('#'):  # Skip empty lines and comments
                    continue
                
                parts = line.split()
                if len(parts) != 3:
                    print(f"Warning: Skipping invalid line {line_num}: '{line}'")
                    print(f"Expected format: from_node to_node capacity")
                    continue
                
                try:
                    from_node = int(parts[0])
                    to_node = int(parts[1])
                    capacity = float(parts[2])
                    
                    # Add nodes to set
                    nodes.add(from_node)
                    nodes.add(to_node)
                    
                    # Add edge
                    edges.append({
                        "from": from_node,
                        "to": to_node,
                        "capacity": capacity
                    })
                    
                except ValueError as e:
                    print(f"Warning: Skipping line {line_num} due to invalid number: '{line}'")
                    continue
    
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found.")
        return None
    except Exception as e:
        print(f"Error reading file: {e}")
        return None
    
    # Convert nodes set to sorted list
    nodes_list = sorted(list(nodes))
    
    # Create the final JSON structure
    graph_data = {
        "nodes": nodes_list,
        "edges": edges
    }
    
    return graph_data

def save_json(data, output_file):
    """Save the graph data to a JSON file."""
    try:
        with open(output_file, 'w') as file:
            json.dump(data, file, indent=2)
        print(f"Successfully converted to '{output_file}'")
        return True
    except Exception as e:
        print(f"Error saving JSON file: {e}")
        return False

def main():
    """Main function to handle command line arguments and conversion."""
    if len(sys.argv) < 2:
        print("Usage: python converter.py <input_file.txt> [output_file.json]")
        print("Example: python converter.py graph_data.txt graph_output.json")
        return
    
    input_file = sys.argv[1]
    
    # Generate output filename if not provided
    if len(sys.argv) >= 3:
        output_file = sys.argv[2]
    else:
        # Replace .txt extension with .json, or add .json if no extension
        input_path = Path(input_file)
        if input_path.suffix.lower() == '.txt':
            output_file = str(input_path.with_suffix('.json'))
        else:
            output_file = str(input_path) + '.json'
    
    print(f"Converting '{input_file}' to '{output_file}'...")
    
    # Parse the input file
    graph_data = parse_txt_to_graph(input_file)
    
    if graph_data is None:
        return
    
    # Display summary
    print(f"Found {len(graph_data['nodes'])} unique nodes")
    print(f"Found {len(graph_data['edges'])} edges")
    
    # Save to JSON
    if save_json(graph_data, output_file):
        print("Conversion completed successfully!")
    else:
        print("Conversion failed!")

def create_sample_input():
    """Create a sample input file for testing."""
    sample_data = """# Sample graph data
# Format: from_node to_node capacity
1 51 1
1 72 2
1 77 1
1 78 2
2 90 6
2 92 6
2 158 1
2 159 4
3 113 1
3 69 1
3 71 3
3 77 4
3 89 4
3 91 4
3 158 7
4 47 1"""
    
    with open('sample_input.txt', 'w') as f:
        f.write(sample_data)
    print("Created 'sample_input.txt' for testing")

if __name__ == "__main__":
    # Uncomment the next line to create a sample input file
    # create_sample_input()
    
    main()