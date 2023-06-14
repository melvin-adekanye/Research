import os
import shutil
from sage import *

# The raw and regular graphs
GRPAH6_STRING = []

# Get the K value
GRAPH_K = 5

# Create the path for the graphs to be stored
SOURCE_PATH = f'../graph-{GRAPH_K}/P5 Free'
DESTINATION_PATH = f"{os.getcwd()}/graph-{GRAPH_K}"
GRAPHS_PATH = '/graphs'

# Create the path for the graphs to be stored
TEMP_PATH = f'{os.getcwd()}/temp'


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

def make_graph(type):

    if type == "p4+p1":

        return graphs.PathGraph(4).disjoint_union(Graph(1))

    if type == "compliment_diamond+p1":

        # Create the diamond graph
        diamond = Graph()
        diamond.add_edges([(1, 2), (1, 3), (2, 4), (3, 4)])

        # Create Graph(1)
        graph_1 = Graph(1)

        # Perform disjoint union
        disjoint_union = diamond.disjoint_union(graph_1)

        # Take the complement
        complement = disjoint_union.complement()

        # Print the resulting graph
        return complement

    if type == "c4+p1":

        return graphs.CycleGraph(4).disjoint_union(Graph(1))

    if type == "chair":

        return Graph([
            ('A', 'B'),
            ('B', 'C'),
            ('C', 'D'),
            ('C', 'E'),
        ])

    if type == "bull":

        return Graph([
            ('A', 'B'),
            ('A', 'C'),
            ('C', 'B'),
            ('B', 'E'),
            ('C', 'F'),
        ])

    if type == "k5":

        # Create the K5 graph
        k5_graph = Graph()
        k5_graph.add_edges([(0, 1), (0, 2), (0, 3), (0, 4),
                           (1, 2), (1, 3), (1, 4), (2, 3), (2, 4), (3, 4)])

        # Compute the complement of the K5 graph
        complement_graph = k5_graph.complement()

        # Show the complement graph
        return complement_graph

    if type == "dart":

        return Graph([
            ('A', 'B'),
            ('B', 'C'),
            ('B', 'D'),
            ('D', 'C'),
        ]).disjoint_union(Graph(1))

    if type == "compliment_p3+2p1":

        return Graph([
            ('A', 'B'),
            ('B', 'C'),
            ('B', 'D'),
            ('D', 'C'),
        ]).join(Graph(1))

    if type == "w4":

        return Graph([
            ('A', 'B'),
            ('B', 'C'),
            ('C', 'D'),
            ('D', 'A'),
        ]).join(Graph(1))

    if type == "k5+e":

        return Graph([
            ('A', 'B'),
            ('A', 'E'),
            ('B', 'C'),
            ('C', 'D'),
            ('C', 'A'),
            ('D', 'A'),
            ('D', 'E'),
            ('C', 'E'),
            ('B', 'E'),
        ])


SOURCE_DATA = [
    ("P4 + P1", make_graph("p4+p1"), []),
    ("Compliment Diamond + P1 Free", make_graph("compliment_diamond+p1"),  []),
    ("C4 + P1 Free", make_graph("c4+p1"), []),
    ("K5 Free", make_graph("k5"), []),
    ("Dart", make_graph("dart"), []),
    ("Compliment P3 + 2P1 Free", make_graph("compliment_p3+2p1"), []),
    ("W4 Free", make_graph("w4"), []),
    ("K5 + E Free", make_graph("k5+e"), []),
]

if GRAPH_K >= 6:
    SOURCE_DATA.append(("Chair Free", make_graph("chair"), []))
    SOURCE_DATA.append(("Bull Free", make_graph("bull"), []))

# Before saving
# Create paths if they doesn't exist
print('. . . Creating TEMP and DESTENATION directory')
path_manager(TEMP_PATH, DESTINATION_PATH)

for data in SOURCE_DATA:

    path, _, _ = data

    path_manager(TEMP_PATH + "/" + path)
    path_manager(DESTINATION_PATH + "/" + path)


def free_manager(graph, graph6_string, filename):

    for data in SOURCE_DATA:

        path, sub_graph, destination_array = data

        if is_sub_graph_free(graph, sub_graph):

            destination_array.append(
                (graph6_string, filename))

        # Save the graph
        save(TEMP_PATH + "/" + path, graph6_string, filename)


# Is sub graph free
def is_sub_graph_free(graph, sub_graph):

    sub_search = graph.subgraph_search(sub_graph, induced=True)

    # Return True or False
    return (sub_search == None)


# The save function
def save(SAVE_PATH, string, filename):

    # Store this graph in the grpahs folder
    f = open(
        f'{SAVE_PATH}/{filename}', "a+")

    # Write to file
    f.write(f'{string}\n')

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

    # Progress report
    if (index % int(0.1 * len(GRPAH6_STRING))) == 0:

        print(
            f'{index} out of {len(GRPAH6_STRING)} Graphs')

    # Create graphs and get details
    graph = Graph(graph6_string)

    # Is this a H_FREE free graph
    free_manager(graph, graph6_string, filename)


# Final save to DESTINATION PATH
for data in SOURCE_DATA:

    path, sub_graph, destination_array = data

    for destination in destination_array:

        graph6_string, filename = destination

        # Save the graph
        save(DESTINATION_PATH + "/" + path, graph6_string, filename)


# Before saving
# Create paths if they doesn't exist
path_manager(TEMP_PATH)


print(
    f'. . . Success! Saved 👯')

shutil.rmtree(TEMP_PATH)