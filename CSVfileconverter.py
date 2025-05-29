import os
import json
from collections import defaultdict
import re

def parse_filename_to_rating(filename):
    """
    Parse filename to extract SOURCE, TARGET, RATING, TIME.
    Expected format: SOURCE,TARGET,RATING,TIME or similar variations
    
    Args:
        filename (str): The filename containing the rating data
    
    Returns:
        tuple: (source, target, rating, time) or None if parsing fails
    """
    
    # Remove file extension if present
    name_without_ext = os.path.splitext(filename)[0]
    
    # Try different separators that might be used in filenames
    separators = [',', '_', '-', '.']
    
    for sep in separators:
        parts = name_without_ext.split(sep)
        if len(parts) >= 4:
            try:
                source = int(parts[0])
                target = int(parts[1])
                rating = int(parts[2])
                time = int(parts[3])
                
                # Validate rating range
                if -10 <= rating <= 10:
                    return (source, target, rating, time)
            except (ValueError, IndexError):
                continue
    
    # If standard separators don't work, try to extract numbers using regex
    numbers = re.findall(r'-?\d+', name_without_ext)
    if len(numbers) >= 4:
        try:
            source = int(numbers[0])
            target = int(numbers[1])
            rating = int(numbers[2])
            time = int(numbers[3])
            
            # Validate rating range
            if -10 <= rating <= 10:
                return (source, target, rating, time)
        except (ValueError, IndexError):
            pass
    
    return None

def process_filenames_to_network_json(folder_path, output_file_path=None, include_negative_ratings=False):
    """
    Process filenames in a folder to extract rating data and convert to network JSON format.
    
    Args:
        folder_path (str): Path to the folder containing files with rating data in names
        output_file_path (str, optional): Path to save the JSON output
        include_negative_ratings (bool): Whether to include edges with negative ratings
    
    Returns:
        dict: Network data in JSON format
    """
    
    # Set to store all unique nodes
    nodes_set = set()
    
    # Dictionary to handle duplicate edges (same source-target pair)
    edge_dict = defaultdict(list)
    
    # Statistics
    files_processed = 0
    files_failed = 0
    total_records = 0
    
    try:
        if not os.path.exists(folder_path):
            print(f"Error: Folder '{folder_path}' not found.")
            return None
            
        files = os.listdir(folder_path)
        print(f"Found {len(files)} files to process...")
        
        for filename in files:
            file_path = os.path.join(folder_path, filename)
            
            # Skip directories
            if os.path.isdir(file_path):
                continue
            
            # Parse the filename
            rating_data = parse_filename_to_rating(filename)
            
            if rating_data:
                source, target, rating, time = rating_data
                
                # Add nodes to set
                nodes_set.add(source)
                nodes_set.add(target)
                
                # Store rating for this edge (with timestamp for potential sorting)
                edge_dict[(source, target)].append({
                    'rating': rating,
                    'time': time,
                    'filename': filename
                })
                
                files_processed += 1
                total_records += 1
                
                if files_processed % 1000 == 0:  # Progress indicator for large datasets
                    print(f"Processed {files_processed} files...")
            else:
                files_failed += 1
                if files_failed <= 10:  # Show first 10 failed files
                    print(f"Warning: Could not parse filename: {filename}")
                elif files_failed == 11:
                    print("... (suppressing further filename parsing warnings)")
    
    except Exception as e:
        print(f"Error accessing folder: {e}")
        return None
    
    print(f"\nProcessing complete:")
    print(f"- Files successfully processed: {files_processed}")
    print(f"- Files failed to parse: {files_failed}")
    print(f"- Total rating records: {total_records}")
    print(f"- Unique nodes: {len(nodes_set)}")
    print(f"- Unique node pairs: {len(edge_dict)}")
    
    # Convert nodes set to sorted list
    nodes = sorted(list(nodes_set))
    
    # Process edges - handle multiple ratings for same edge pair
    edges = []
    
    for (source, target), ratings_list in edge_dict.items():
        # Sort by timestamp to get chronological order
        ratings_list.sort(key=lambda x: x['time'])
        
        # Strategy options for handling multiple ratings:
        
        # Option 1: Use average rating
        avg_rating = sum(r['rating'] for r in ratings_list) / len(ratings_list)
        capacity = int(round(avg_rating))
        
        # Option 2: Use latest rating (uncomment to use instead)
        # capacity = ratings_list[-1]['rating']
        
        # Option 3: Use first rating (uncomment to use instead)
        # capacity = ratings_list[0]['rating']
        
        # Option 4: Use sum of ratings (uncomment to use instead)
        # capacity = sum(r['rating'] for r in ratings_list)
        
        # Filter edges based on rating
        should_include = True
        if not include_negative_ratings and capacity <= 0:
            should_include = False
        
        if should_include:
            edge_data = {
                "from": source,
                "to": target,
                "capacity": capacity
            }
            
            # Optionally add metadata
            edge_data["rating_count"] = len(ratings_list)
            if len(ratings_list) > 1:
                edge_data["rating_range"] = [min(r['rating'] for r in ratings_list), 
                                           max(r['rating'] for r in ratings_list)]
            
            edges.append(edge_data)
    
    # Create the final JSON structure
    network_data = {
        "nodes": nodes,
        "edges": edges
    }
    
    # Add metadata
    network_data["metadata"] = {
        "total_files_processed": files_processed,
        "total_rating_records": total_records,
        "unique_node_pairs": len(edge_dict),
        "include_negative_ratings": include_negative_ratings
    }
    
    # Save to file if path provided
    if output_file_path:
        try:
            with open(output_file_path, 'w', encoding='utf-8') as jsonfile:
                json.dump(network_data, jsonfile, indent=2)
            print(f"\nJSON file saved to: {output_file_path}")
        except Exception as e:
            print(f"Error saving JSON file: {e}")
    
    return network_data

def create_clean_network_json(network_data, output_file_path=None):
    """
    Create a clean version without metadata, matching your exact format.
    """
    if not network_data:
        return None
    
    clean_data = {
        "nodes": network_data["nodes"],
        "edges": []
    }
    
    # Remove extra metadata from edges
    for edge in network_data["edges"]:
        clean_edge = {
            "from": edge["from"],
            "to": edge["to"],
            "capacity": edge["capacity"]
        }
        clean_data["edges"].append(clean_edge)
    
    if output_file_path:
        try:
            with open(output_file_path, 'w', encoding='utf-8') as jsonfile:
                json.dump(clean_data, jsonfile, indent=2)
            print(f"Clean JSON file saved to: {output_file_path}")
        except Exception as e:
            print(f"Error saving clean JSON file: {e}")
    
    return clean_data

def print_network_stats(network_data):
    """Print basic statistics about the network."""
    if not network_data:
        return
        
    print(f"\nNetwork Statistics:")
    print(f"- Number of nodes: {len(network_data['nodes'])}")
    print(f"- Number of edges: {len(network_data['edges'])}")
    
    if network_data['nodes']:
        print(f"- Node range: {min(network_data['nodes'])} to {max(network_data['nodes'])}")
    
    if network_data['edges']:
        capacities = [edge['capacity'] for edge in network_data['edges']]
        print(f"- Capacity range: {min(capacities)} to {max(capacities)}")
        print(f"- Average capacity: {sum(capacities) / len(capacities):.2f}")
        
        # Show rating distribution
        positive_edges = sum(1 for c in capacities if c > 0)
        negative_edges = sum(1 for c in capacities if c < 0)
        zero_edges = sum(1 for c in capacities if c == 0)
        
        print(f"- Positive capacity edges: {positive_edges}")
        print(f"- Negative capacity edges: {negative_edges}")
        print(f"- Zero capacity edges: {zero_edges}")

def sample_filename_formats(folder_path, sample_size=10):
    """
    Show sample filenames to help debug parsing issues.
    """
    try:
        files = os.listdir(folder_path)[:sample_size]
        print(f"\nSample filenames from folder:")
        for i, filename in enumerate(files, 1):
            rating_data = parse_filename_to_rating(filename)
            if rating_data:
                source, target, rating, time = rating_data
                print(f"{i:2d}. {filename} -> Source: {source}, Target: {target}, Rating: {rating}, Time: {time}")
            else:
                print(f"{i:2d}. {filename} -> FAILED TO PARSE")
    except Exception as e:
        print(f"Error sampling filenames: {e}")

# Example usage
if __name__ == "__main__":
    # Replace with your folder path
    folder_path = "data1"
    json_output = "RWD3783_22650oR.json"
    clean_json_output = "RWD3783_22650.json"
    
    print("Analyzing sample filenames...")
    sample_filename_formats(folder_path)
    
    print(f"\nProcessing filenames in folder: {folder_path}")
    
    # Process filenames to create network
    # Set include_negative_ratings=True if you want to include negative ratings as edges
    network_data = process_filenames_to_network_json(
        folder_path, 
        json_output, 
        include_negative_ratings=False  # Change to True if you want negative ratings
    )
    
    if network_data:
        print_network_stats(network_data)
        
        # Create clean version matching your exact format
        clean_data = create_clean_network_json(network_data, clean_json_output)
        
        # Print first few edges as example
        print(f"\nFirst 10 edges:")
        for i, edge in enumerate(network_data['edges'][:10]):
            print(f"  {edge}")
        
        print(f"\nFiles saved:")
        print(f"- Full data with metadata: {json_output}")
        print(f"- Clean format: {clean_json_output}")