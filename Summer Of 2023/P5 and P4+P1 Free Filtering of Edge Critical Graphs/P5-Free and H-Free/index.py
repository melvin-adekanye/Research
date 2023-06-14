import os
import shutil
from sage import *

# Store all the H_FREE_H_FREE_CIRCULANT_GRAPHS
H_FREE_CIRCULANT_GRAPHS = []

# The raw and regular graphs
GRPAH6_STRING = []

# Get the K value
GRAPH_K = 7

# Create the path for the graphs to be stored
SOURCE_PATH = f'../graph-{GRAPH_K}/P5 Free'
DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}"
GRAPHS_PATH = '/graphs'

# Create the path for the graphs to be stored
H_FREE_SAVE_PATHS = ["", ""] # f'{DESTINATION_PATH}/H Free'
TEMP_H_FREE_SAVE_PATH = f'{DESTINATION_PATH}/temp/H Free'


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


def is_H_free(graph):

    # Is H free boolean
    is_H_free_boolean = True

    def is_H_FREE_free(graph):
        for v in graph.vertices():
            neighbors_v = graph.neighbors(v)
            # Check if the vertex v has at least 3 neighbors
            if len(neighbors_v) >= 3:
                # Check if v is connected to every pair of its neighbors
                for i in range(len(neighbors_v)):
                    for j in range(i + 1, len(neighbors_v)):
                        u = neighbors_v[i]
                        w = neighbors_v[j]
                        if not graph.has_edge(u, w):
                            # Check if there is an isolated vertex x
                            for x in graph.vertices():
                                if x != v and not graph.has_edge(x, u) and not graph.has_edge(x, w):
                                    return False
        return True

    # Is p4Up1 free boolean flag function [Corrected]
    def is_p4Up1_free(G):

        P4UP1 = graphs.PathGraph(4).disjoint_union(Graph(1))
        sub_search = G.subgraph_search(P4UP1, induced=True)

        # Return True or False
        return (sub_search == None)


    def complement_diamond_p1_free(graph):
        complement_graph = graph.complement()  # Create the complement graph
        for v in complement_graph.vertices():
            neighbors = set(complement_graph.neighbors(v))
            for u in neighbors:
                # Check for a diamond
                for w in neighbors.intersection(set(complement_graph.neighbors(u))):
                    if w != v and w not in complement_graph.neighbors(v):
                        # Check for a P1
                        if not any(w in complement_graph.neighbors(x) for x in complement_graph.neighbors(v)):
                            return False
        return True

    def is_c4_p1_free(graph):
        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 3:
                # Check for a C4
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for a P1
                            if any(w in graph.neighbors(x) for x in graph.neighbors(v)):
                                return False
        return True

    # finite k ≤ 5, unknown k ≥ 6
    def is_chair_free(graph):

        # Special checks
        if GRAPH_K < 6:

            return None

        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 3:
                # Check for a chair
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for the pendant vertex
                            if len(set(graph.neighbors(w)) - {v, u}) == 1:
                                return False
        return True

    # finite k = 5, unknown k ≥ 6
    def is_bull_free(graph):

        # Special checks
        if GRAPH_K < 6:

            return None

        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 2:
                # Check for a bull
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for the horns
                            for x in graph.neighbors(w):
                                if x != v and x != u and x not in neighbors:
                                    return False
        return True

    # infinite k = 5, unknown k ≥ 6
    def is_k5_free(graph):

        # Special checks
        if GRAPH_K < 6:

            return None

        k5 = Graph("D~{") # Graph([(0, 1), (0, 2), (0, 3), (0, 4), (1, 2), (1, 3), (1, 4), (2, 3), (2, 4), (3, 4)])
        return not graph.subgraph_search(k5, induced=True)


    def is_dart_free(graph):
        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 3:
                # Check for a dart
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for the tail
                            if len(set(graph.neighbors(w)) - {v}) == 1:
                                return False
        return True

    def is_complement_p3_2p1_free(graph):
        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 2:
                # Check for P3
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for 2P1
                            isolated_vertices = set(
                                graph.vertices()) - neighbors
                            if len(isolated_vertices) >= 2:
                                return False
        return True

    def is_w4_free(graph):
        for v in graph.vertices():
            neighbors = set(graph.neighbors(v))
            if len(neighbors) >= 3:
                # Check for W4
                for u in neighbors:
                    for w in neighbors.intersection(set(graph.neighbors(u))):
                        if w != v and w not in graph.neighbors(v):
                            # Check for central vertex
                            central_vertex = neighbors - {u, w}
                            if len(central_vertex.intersection(set(graph.neighbors(w)))) >= 2:
                                return False
        return True

    # Questionable?!?
    def is_k5_e_free(graph):
        k5 = Graph("D~{")
        return not any(graph.subgraph_search(k5).count() == 5 for k5_e_subgraph in graph.subgraph_search_iterator(k5) if k5_e_subgraph == 9)

    checks = [
              is_p4Up1_free,
              complement_diamond_p1_free,
              is_c4_p1_free,
              is_chair_free,
              is_k5_free,
              is_bull_free,
              is_dart_free,
              is_complement_p3_2p1_free,
              is_w4_free,
              is_k5_e_free,
              ]

    for check in checks:

        if check(graph) is False:

            is_H_free_boolean = False

    return is_H_free_boolean


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
path_manager(TEMP_H_FREE_SAVE_PATH)

print('. . . Analyzing graph data')
# Loop through all the GRPAH6_STRING
for (index, data) in enumerate(GRPAH6_STRING):

    # Get the graph6 string and file name
    graph6_string, filename = data

    # Progress report
    if (index % int(0.1 * len(GRPAH6_STRING))) == 0:

        print(
            f'{index} out of {len(GRPAH6_STRING)} — {len(H_FREE_CIRCULANT_GRAPHS)} Graphs Added')

    # Create graphs and get details
    graph = Graph(graph6_string)

    # Is this a H_FREE free graph
    if is_H_free(graph):

        # iF so, append it to the H_FREE_H_FREE_CIRCULANT_GRAPHS
        H_FREE_CIRCULANT_GRAPHS.append(
            (graph6_string, filename))

        # Save the graph
        save(TEMP_H_FREE_SAVE_PATH, graph6_string, filename)


# Before saving
# Create paths if they doesn't exist
path_manager(H_FREE_SAVE_PATH)

# Print the generated graphs
for graph in H_FREE_CIRCULANT_GRAPHS:

    graph6_string, filename = graph

    # Save the graph
    save(H_FREE_SAVE_PATH, graph6_string, filename)


print(
    f'. . . Success! Saved to {H_FREE_SAVE_PATH}')
