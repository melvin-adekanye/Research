import os
import shutil
from sage import *
from sage.graphs.graph_coloring import vertex_coloring

# The raw and regular graphs
GRPAH6_STRING = []

# Get the K value
GRAPH_K = 7

# Create the path for the graphs to be stored
SOURCE_PATH = f'{os.getcwd()}/DATA'
CRITICAL_DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}/critical"
_2P2_DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}/2p2-free"
CRITICAL_and_2P2_DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}/critical and 2p2-free"


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
            # shutil.rmtree(path)
            # path_manager(path)
            pass


# Before saving
# Create paths if they doesn't exist
print('. . . Creating DESTENATION directory')
path_manager(CRITICAL_DESTINATION_PATH,
             _2P2_DESTINATION_PATH, CRITICAL_and_2P2_DESTINATION_PATH)


# The OG critical check
def critical_check(G, k=GRAPH_K):
    V = G.vertices()
    chi = G.chromatic_number()
    if (chi != k):
        return False

    for v in V:
        # creates local copy of G so we can delete vertices and maintain G's structure
        H = Graph(G)
        H.delete_vertex(v)
        if vertex_coloring(H, k=k-1, value_only=True) == False:
            return False
    return True


# A 2p2 free function
def is_2P2_free(G):

    _2p2 = Graph()
    _2p2.add_edges([('A', 'B'), ('C', 'D')])

    sub_search = G.subgraph_search(_2p2, induced=True)

    # Return True or False
    return (sub_search == None)


# The save function
def save(SAVE_PATH, complement_graph6_string, graph6_string, filename):

    # Store this graph in the grpahs folder
    f = open(
        f'{SAVE_PATH}/{filename}', "a+")

    # Write to file
    f.write(f'c: {complement_graph6_string} o: {graph6_string} \n')

    # CLose the file save
    f.close()


# Read in the 5-critical graphs
print('. . . Gathering graph data files from raw graphs folder')
directory = os.fsencode(SOURCE_PATH)
for file in os.listdir(directory):

    filename = os.fsdecode(file)

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


print('. . . Analyzing graph data')
# Loop through all the GRPAH6_STRING
for (index, data) in enumerate(GRPAH6_STRING):

    # Get the graph6 string and file name
    graph6_string, filename = data

    print(
        f'{index} out of {len(GRPAH6_STRING)} Graphs')

    # Get the complement of the graph
    complement_graph = Graph(graph6_string).complement()

    # Get the graph string of the complement graph
    complement_graph6_string = complement_graph.graph6_string()

    is_free_from_2p2 = False
    is_critical = False

    # print out the vertex being deleted
    print(f'. . . critical_check() - {graph6_string}')

    # If is 2p2 free
    if is_2P2_free(complement_graph) == True:

        is_free_from_2p2 = True

        # Check if it is 2p2 free
        save(_2P2_DESTINATION_PATH, complement_graph6_string,
             graph6_string, filename)

    # If it is_critical
    if critical_check(complement_graph) == True:

        is_critical = True

        # Check if it is 2p2 free
        save(CRITICAL_DESTINATION_PATH,
             complement_graph6_string, graph6_string, filename)

    if is_free_from_2p2 and is_critical:

        # Check if it's both 2p2 and k-critical
        save(CRITICAL_and_2P2_DESTINATION_PATH,
             complement_graph6_string, graph6_string, filename)


print(
    f'. . . Success! Saved 👯')
