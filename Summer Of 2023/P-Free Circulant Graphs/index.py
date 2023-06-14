import os
import shutil
from sage import *

# Parallelism().set(nproc=8)

# Store all the P5_FREE_CIRCULANT_GRAPHS
P5_FREE_CIRCULANT_GRAPHS = []

# Store all the P4UP1_FREE_CIRCULANT_GRAPHS
P4UP1_FREE_CIRCULANT_GRAPHS = []

# The raw and regular graphs
RAW_GRAPHS = []

print(
    f'**** STARTING - Finding PX Free Circulant Graphs ****')

# Create the path for the graphs to be stored
SOURCE_PATH = f'{os.getcwd()}/../DATA/raw graphs'

# Create the path for the graphs to be stored
P5_SAVE_PATH = f'{os.getcwd()}/P5 Free'
P4UP1_SAVE_PATH = f'{os.getcwd()}/P4+P1 Free'
GRAPHS_PATH = '/graphs'
GRAPHS_PARAMS_PATH = '/graphs params'
TEMP_PATH = f"{os.getcwd()}/temp"
TEMP_P5_SAVE_PATH = f'{TEMP_PATH}/P5 Free'
TEMP_P4UP1_SAVE_PATH = f'{TEMP_PATH}/P4+P1 Free'


# Define the path manager
def path_manager(*paths):

    for path in paths:

        # Check whether the specified path exists or not
        isExist = os.path.exists(path)

        if not isExist:

            # Create a new directory because it does not exist
            os.makedirs(path)
            os.makedirs(path + GRAPHS_PATH)
            os.makedirs(path + GRAPHS_PARAMS_PATH)
            print(f"The new directory {path} has been created!")

        else:

            # Clear file path for new data
            shutil.rmtree(path)
            path_manager(path)


# The circulent generator
def circulant(n, L):

    E = []

    for i in range(n):

        for j in range(i+1, n):

            if (((i-j) % n) in L):

                if ({i, j} not in E):

                    E.append({i, j})

    # Return the graph
    return E


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


# Convert array to string
def convert_array_to_string(array):

    total_string = ''

    # traverse in the string
    for (i, string) in enumerate(array):

        total_string += str(string)

        # Don't add space after last character
        if i < len(array)-1:

            total_string += ' '

    # Return the total string
    return total_string


# The save function
def save(SAVE_PATH, string, raw_string, order, chromatic_number):

    DEFAULT_GRAPH6_STRING_SAVE_PATH = f'{SAVE_PATH}{GRAPHS_PATH}'
    DEFAULT_PARAMS_SAVE_PATH = f'{SAVE_PATH}{GRAPHS_PARAMS_PATH}'

    # Store this graph in the grpahs folder
    f = open(
        f'{DEFAULT_GRAPH6_STRING_SAVE_PATH}/circ{order}_chi{chromatic_number}.txt', "a+")
    e = open(
        f'{DEFAULT_PARAMS_SAVE_PATH}/circ{order}_chi{chromatic_number}_params.txt', "a+")

    # Write to file
    f.write(f'{string}\n')
    e.write(f'{raw_string}\n')

    # CLose the file save
    f.close()
    e.close()


print('. . . Gathering graph data files from raw graphs folder')
directory = os.fsencode(SOURCE_PATH)
for file in os.listdir(directory):

    filename = os.fsdecode(file)

    print(filename)

    # Defien a path to the graph (may not actually exist)
    path = f'{SOURCE_PATH}/{filename}'
    
    try:
            
        # Order
        order = int(filename.split(".")[0].split('circ')[1])

        # Number of the parameters
        number_of_parameters = int(filename.split(".")[1].split('.txt')[0])

        # Try: If the GRAPH_PATH does exist
        try:

            # Open up and read the file graph
            f = open(path, "r")

            # FOr every line in the file
            for line in f:

                # Attain the raw graph numbers
                string = line.split(None)

                string = list(map(int, string))

                # Append a list to the raw graph
                RAW_GRAPHS.append([order, string])

            # CLose the file
            f.close()

        except:

            # If the graph path doesn't exist skip
            pass

    except:

        # If the graph path doesn't exist skip
        pass


# Before saving
# Create paths if they doesn't exist
print('. . . Creating TEMP directory')
path_manager(TEMP_P5_SAVE_PATH, TEMP_P4UP1_SAVE_PATH)


print('. . . Analyzing graph data')
# For all the raw graphs
for (index, data) in enumerate(RAW_GRAPHS):

    checkpoint = int(0.1 * len(RAW_GRAPHS)) if int(0.1 * len(RAW_GRAPHS)) > 0 else 1

    # Progress report
    if (index % checkpoint) == 0:

        print(f'{index} out of {len(RAW_GRAPHS)}')

    # Define the order
    order = data[0]

    # Define the graph raw string (5 7 10 12)
    graph_raw_string = data[1]

    # Remove array props from the graph_raw_string
    graph6_raw_array_to_string = convert_array_to_string(graph_raw_string)

    # graph = circulant(first_elem, remaining_elems)
    E = circulant(order, graph_raw_string)

    # Create graphs and get details
    graph = Graph(E)
    graph6_string = graph.graph6_string()
    chromatic_number = graph.chromatic_number()

    # Back searching for finding the chromatic number - shortcuts for finding K quickly

    # Is this a p5 free graph
    if is_p5_free(graph):

        # iF so, append it to the P5_FREE_CIRCULANT_GRAPHS
        P5_FREE_CIRCULANT_GRAPHS.append(
            (graph, graph6_string, graph6_raw_array_to_string, order, chromatic_number))

        # Save the graph
        save(TEMP_P5_SAVE_PATH, graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)

    # Check out the p4+p1 free graphs
    if is_p4Up1_free(graph):

        P4UP1_FREE_CIRCULANT_GRAPHS.append(
            (graph, graph6_string, graph6_raw_array_to_string, order, chromatic_number))

        # Save the graph
        save(TEMP_P4UP1_SAVE_PATH, graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)


# Before saving
# Create paths if they doesn't exist
path_manager(P5_SAVE_PATH, P4UP1_SAVE_PATH)

# Print the generated graphs
for graph in P5_FREE_CIRCULANT_GRAPHS:

    # Unpack the graph data
    _, graph6_string, graph6_raw_array_to_string, order, chromatic_number = graph

    # Save the graph
    save(P5_SAVE_PATH, graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)


# Print the generated graphs
for graph in P4UP1_FREE_CIRCULANT_GRAPHS:

    # Unpack the graph data
    _, graph6_string, graph6_raw_array_to_string, order, chromatic_number = graph

    # Save the graph
    save(P4UP1_SAVE_PATH, graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)


print(
    f'. . . Success! Saved to {P5_SAVE_PATH} and {P4UP1_SAVE_PATH}')

shutil.rmtree(TEMP_PATH)