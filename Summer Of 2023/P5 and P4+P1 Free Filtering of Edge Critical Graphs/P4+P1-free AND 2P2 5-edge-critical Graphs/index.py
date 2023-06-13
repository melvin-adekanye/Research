import os
import shutil
from sage import *

# Store all the _2P2_FREE_CIRCULANT_GRAPHS
P4UP1_AND_2P2_FREE_CIRCULANT_GRAPHS = []

# The raw and regular graphs
GRPAH6_STRING = []

# Get the K value
GRAPH_K = 5

# Create the path for the graphs to be stored
SOURCE_PATH = f'../graph-{GRAPH_K}/P4+P1 Free'
DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}"
GRAPHS_PATH = '/graphs'

# Create the path for the graphs to be stored
_2p2_SAVE_PATH = f'{DESTINATION_PATH}/2P2 Free'
TEMP__2p2_SAVE_PATH = f'{DESTINATION_PATH}/temp/2P2 Free'


# Define the path manager
def path_manager(*paths):

    for path in paths:

        # Check whether the specified path exists or not
        isExist = os.path.exists(path)

        if not isExist:

            # Create a new directory because it does not exist
            os.makedirs(path)
            print(f"The new directory {path} has been created!")

        else:

            # Clear file path for new data
            shutil.rmtree(path)
            path_manager(path)


# Read in the 5-critical graphs
print('. . . Gathering graph data files from raw graphs folder')
directory = os.fsencode(SOURCE_PATH)
for file in os.listdir(directory):

    filename = os.fsdecode(file)

    print(f"\n\n====={filename}")

    # Defien a path to the graph (may not actually exist)
    path = f'{SOURCE_PATH}/{filename}'

    # Try: If the GRAPH_PATH does exist
    try:

        # Open up and read the file graph
        f = open(path, "r")

        # FOr every line in the file
        for line in f:

            # Attain the raw graph numbers
            string = line.split(None)[0]

            # Append a list to the raw graph
            GRPAH6_STRING.append((string, filename))

        # CLose the file
        f.close()

    except:

        # If the graph path doesn't exist skip
        pass


# Is p4Up1 free boolean flag function [Corrected]
def is_p4Up1_free(G):

    P4UP1 = graphs.PathGraph(4).disjoint_union(Graph(1))
    sub_search = G.subgraph_search(P4UP1, induced=True)

    # Return True or False
    return (sub_search == None)


def is_2P2_free(G):

    for v in graph.vertices():
        neighbors = set(graph.neighbors(v))
        if len(neighbors) >= 2:
            # Check for 2P2
            for u in neighbors:
                for w in neighbors - {u}:
                    if u != w and not graph.has_edge(u, w):
                        return False
    return True

# The save function
def save(SAVE_PATH, string, filename):

    # Store this graph in the grpahs folder
    f = open(
        f'{SAVE_PATH}/{filename}', "a+")

    # Write to file
    f.write(f'{string}\n')

    # CLose the file save
    f.close()


# Before saving
# Create paths if they doesn't exist
print('. . . Creating TEMP directory')
path_manager(TEMP__2p2_SAVE_PATH)

print('. . . Analyzing graph data')
# Loop through all the GRPAH6_STRING
for (index, data) in enumerate(GRPAH6_STRING):

    # Get the graph6 string and file name
    graph6_string, filename = data

    # Progress report
    if (index % int(0.1 * len(GRPAH6_STRING))) == 0:

        print(f'{index} out of {len(GRPAH6_STRING)} — {len(P4UP1_AND_2P2_FREE_CIRCULANT_GRAPHS)} Graphs Added')

    # Create graphs and get details
    graph = Graph(graph6_string)

    if is_p4Up1_free(graph):

        # Is this a 2p2 free graph
        if is_2P2_free(graph):

            # iF so, append it to the _2P2_FREE_CIRCULANT_GRAPHS
            P4UP1_AND_2P2_FREE_CIRCULANT_GRAPHS.append(
                (graph6_string, filename))

            # Save the graph
            save(TEMP__2p2_SAVE_PATH, graph6_string, filename)


# Before saving
# Create paths if they doesn't exist
path_manager(_2p2_SAVE_PATH)

# Print the generated graphs
for graph in P4UP1_AND_2P2_FREE_CIRCULANT_GRAPHS:

    graph6_string, filename = graph

    # Save the graph
    save(_2p2_SAVE_PATH, graph6_string, filename)


print(
    f'. . . Success! Saved to {_2p2_SAVE_PATH}')
