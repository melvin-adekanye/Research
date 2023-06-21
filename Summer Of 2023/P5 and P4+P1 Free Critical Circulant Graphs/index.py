import os
import shutil
from sage import *

# Parallelism().set(nproc=8)

# Store all the graph strings
GRPAH6_STRING = []

# Define the CHROMATIC_NUMBER "./DATA/graph-[CHROMATIC_NUMBER]"
CHROMATIC_NUMBER = 7

# Graph of study path
GRAPH_OF_STUDY_FILENAME = f'graph-{CHROMATIC_NUMBER}'

print(
    f'****  Finding {GRAPH_OF_STUDY_FILENAME} Free Circulant Critical Graphs ****')

# Create the path for the graphs to be stored
SOURCE_PATH = f'{os.getcwd()}/DATA/{GRAPH_OF_STUDY_FILENAME}/critical graphs'

# Create the path for the graphs to be stored
P5_SAVE_PATH = f'{os.getcwd()}/critical-{GRAPH_OF_STUDY_FILENAME}/P5 Free'
P4UP1_SAVE_PATH = f'{os.getcwd()}/critical-{GRAPH_OF_STUDY_FILENAME}/P4+P1 Free'

TEMP_PATH = f'{os.getcwd()}/temp'
TEMP_P5_SAVE_PATH = f'{TEMP_PATH}/critical-{GRAPH_OF_STUDY_FILENAME}/P5 Free'
TEMP_P4UP1_SAVE_PATH = f'{TEMP_PATH}/critical-{GRAPH_OF_STUDY_FILENAME}/P4+P1 Free'

# Define all the temp paths
TEMP_PATHs = [TEMP_PATH, TEMP_P5_SAVE_PATH,
              TEMP_P4UP1_SAVE_PATH]
SAVE_PATHs = [P5_SAVE_PATH, P4UP1_SAVE_PATH]

# Store all the P5_FREE_CIRCULANT_CRITICAL_GRAPHS
P5_FREE_CIRCULANT_CRITICAL_GRAPHS = []

# Store all the P4UP1_FREE_CIRCULANT_CRITICAL_GRAPHS
P4UP1_FREE_CIRCULANT_CRITICAL_GRAPHS = []


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


# Is p5 free boolean flag function
def is_p5_free(G):

    P5 = graphs.PathGraph(5)
    sub_search = G.subgraph_search(P5, induced=True)

    # Return True or False
    return (sub_search == None)


# Is p4Up1 free boolean flag function [Corrected]
def is_p4Up1_free(G):

    P4UP1 = graphs.PathGraph(4).disjoint_union(Graph(1))
    sub_search = G.subgraph_search(P4UP1, induced=True)

    # Return True or False
    return (sub_search == None)


# The save function
def save(SAVE_PATH, string, filename):

    DEFAULT_GRAPH6_STRING_SAVE_PATH = f'{SAVE_PATH}'

    # Store this graph in the grpahs folder
    f = open(
        f'{DEFAULT_GRAPH6_STRING_SAVE_PATH}/{filename}.g6', "a+")

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


# Before saving
# Create paths if they doesn't exist
print('. . . Creating TEMP directory')
path_manager(*TEMP_PATHs)


print('. . . Analyzing graph data')
# Loop through all the GRPAH6_STRING
for (index, data) in enumerate(GRPAH6_STRING):

    # Progress report
    if (index % int(0.05 * len(GRPAH6_STRING))) == 0:

        print(f'{index} out of {len(GRPAH6_STRING)}')

    # Define the order
    graph6_string, filename = data

    # Create graphs and get details
    graph = Graph(graph6_string)

    is_it_p5_free = is_p5_free(graph)
    is_it_p4Up1_free = is_p4Up1_free(graph)

    # Is this a p5 free graph
    if is_it_p5_free:

        # iF so, append it to the P5_FREE_CIRCULANT_CRITICAL_GRAPHS
        P5_FREE_CIRCULANT_CRITICAL_GRAPHS.append((graph6_string, filename))

        # Save the graph
        save(TEMP_P5_SAVE_PATH, graph6_string, filename)

    # Check out the p4+p1 free graphs
    if is_it_p4Up1_free:

        P4UP1_FREE_CIRCULANT_CRITICAL_GRAPHS.append((graph6_string, filename))

        # Save the graph
        save(TEMP_P4UP1_SAVE_PATH, graph6_string, filename)

    print(f"{graph6_string} - P5-Free? {is_it_p5_free} - P4+P1-Free? {is_it_p4Up1_free}")

# Before saving
# Create paths if they doesn't exist
path_manager(*SAVE_PATHs)


# Define the save manager
def save_manager(SAVE_PATH, ARRAY_TO_SAVE):

    # Print the generated graphs
    for graph in ARRAY_TO_SAVE:

        # Unpack the graph data
        graph6_string, filename = graph

        # Save the graph
        save(SAVE_PATH, graph6_string, filename)


# Call the save manager
save_manager(P5_SAVE_PATH, P5_FREE_CIRCULANT_CRITICAL_GRAPHS)
save_manager(P4UP1_SAVE_PATH, P4UP1_FREE_CIRCULANT_CRITICAL_GRAPHS)

print(
    f'. . . Success! Saved {GRAPH_OF_STUDY_FILENAME}')

# Delete the temp folder
shutil.rmtree(TEMP_PATH)
