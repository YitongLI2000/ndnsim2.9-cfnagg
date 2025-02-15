import networkx as nx
import matplotlib.pyplot as plt
import configparser


def parse_agg_tree(file_path):
    """
    Parse aggTree.txt file and build a directed graph.
    - Combine all sub-trees under a single root node 'con0'.
    - Remove '-> round x' labels completely.
    """
    G = nx.DiGraph()  # Directed graph for logical tree
    root_node = "con0"  # Define the single root node

    with open(file_path, 'r') as f:
        for line in f:
            if ':' in line:
                # Parse parent and children
                parent, children = line.split(':')
                parent = parent.strip()
                children = children.strip().split()
                for child in children:
                    G.add_edge(parent, child)  # Add parent-child relationship
            elif '->' in line:
                # Parse sub-tree roots for 'con0'
                _, children = line.split(':')  # Ignore the "-> round x" part
                children = children.strip().split()
                for child in children:
                    G.add_edge(root_node, child)  # Connect 'con0' to subtree roots
    return G


def draw_logical_tree(G, output_file):
    """
    Draw the logical tree with a hierarchical layout.
    - Use graphviz_layout for a clear tree structure.
    """
    # Use Graphviz layout for hierarchical arrangement
    pos = nx.drawing.nx_agraph.graphviz_layout(G, prog="dot")  # 'dot' ensures a tree layout

    # Draw the graph
    plt.figure(figsize=(12, 8))  # Set figure size
    nx.draw(
        G,
        pos,
        with_labels=True,
        node_size=800,
        node_color="lightgreen",
        font_size=10,
        font_color="black",
        edge_color="gray",
        arrows=True
    )
    plt.title("Logical Tree Structure (aggTree)", fontsize=16)
    plt.savefig(output_file)  # Save the graph as an image
    plt.close()  # Close the figure


def read_config(config_file):
    """
    Read the configuration file to get the topology type.
    """
    config = configparser.ConfigParser()
    config.read(config_file)
    return config['General']['TopologyType']


topo_type = read_config("../simulation_settings/config.ini")
topo_file = ""

# Update the path
if topo_type == "BinaryTree8":
    topo_file = "../../results/logs/BinaryTreeTopology8.txt"
elif topo_type == "BinaryTree16":
    topo_file = "../../results/logs/BinaryTreeTopology16.txt"
elif topo_type == "BinaryTree32":
    topo_file = "../../results/logs/BinaryTreeTopology32.txt"
elif topo_type == "ISP":
    topo_file = "../../results/logs/ISPTopology.txt"     
elif topo_type == "DCN":
    topo_file = "../../results/logs/DataCenterTopology.txt"    

# Visualize aggTree with hierarchical layout
#topo_tree = parse_agg_tree(topo_file)


# Draw and save the logical tree
#draw_logical_tree(topo_tree, "../../results/logs/topo_logical.png")

# Visualize aggTree with hierarchical layout
agg_tree_file = "../../results/logs/aggTree.txt"
agg_tree = parse_agg_tree(agg_tree_file)

# Draw and save the logical tree
draw_logical_tree(agg_tree, "../../results/logs/agg_tree_logical.png")
