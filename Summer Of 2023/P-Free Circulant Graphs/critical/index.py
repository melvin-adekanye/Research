import os
import shutil
from sage import *
from sage.graphs.graph_coloring import vertex_coloring

# Parallelism().set(nproc=8)

# Store all the P5_FREE_CRITICAL_GRAPHS
P5_FREE_CRITICAL_GRAPHS = []

# Store all the P4UP1_FREE_CRITICAL_GRAPHS
P4UP1_FREE_CRITICAL_GRAPHS = []

# The raw and regular graphs
RAW_GRAPHS = []

MIN_VERTICES = 5  # Default=5 || In DATA/raw_graphs you'll find "circ5.x" graphs
MAX_VERTICES = 50  # Default=50 || In DATA/raw_graphs you'll find "circ50.x" graphs

GRAPHS_PATH = '/graphs'
GRAPHS_PARAMS_PATH = '/graphs params'

# Create the path for the graphs to be stored
P5_CRITICAL_SAVE_PATH = f'{os.getcwd()}/P5 Free'
P4UP1_CRITICAL_SAVE_PATH = f'{os.getcwd()}/P4+P1 Free'

TEMP_PATH = f'{os.getcwd()}/temp'

TEMP_CRITICAL_P5_CRITICAL_SAVE_PATH = f'{TEMP_PATH}/P5 Free'
TEMP_CRITICAL_P4UP1_CRITICAL_SAVE_PATH = f'{TEMP_PATH}/P4+P1 Free'

print(
    f'**** STARTING AT {MIN_VERTICES} to {MAX_VERTICES} Vertices - Finding *Critical* PX Free Circulant Graphs ****')

# An array of the SOURCE_PATHS
SOURCE_PATHS = [(f'{os.getcwd()}/../P5 Free{GRAPHS_PARAMS_PATH}', P5_FREE_CRITICAL_GRAPHS, TEMP_CRITICAL_P5_CRITICAL_SAVE_PATH, P5_CRITICAL_SAVE_PATH),
                (f'{os.getcwd()}/../P4+P1 Free{GRAPHS_PARAMS_PATH}', P4UP1_FREE_CRITICAL_GRAPHS, TEMP_CRITICAL_P4UP1_CRITICAL_SAVE_PATH, P4UP1_CRITICAL_SAVE_PATH)]


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


<<<<<<< HEAD
=======
<<<<<<< HEAD
# The OG critical check
def critical_check(G, k):
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
=======
>>>>>>> master
# Critical check. Takes in the graph6_string, graph raw string and the chromatic number
def critical_check(graph, order, chromatic_number):

    # Set the original graph before messing with it
    original_graph = Graph(graph)

    # A flag to check if a graph is critical
    is_critical = True

    # Iterate through every vertice in graphs vertices
    for index in range(order):

        # print out the vertex being deleted
        print(f'. . . Deleting Vertice-{index} of {order}')

        # Remove the vertex at index
        graph.delete_vertex(index)

        # print out the vertex being deleted
        print(f'. . . Finding Vertex Coloring')

        # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
        if vertex_coloring(graph, k=chromatic_number-1, value_only=True) == False:

            # Is critical is false
            is_critical = False

            # Not critical, lets break the loop
            break

    # Reset the graph to replace deleted vertex
    graph = Graph(original_graph)

    # Return is_critical
    return is_critical
<<<<<<< HEAD
=======
>>>>>>> origin/main
>>>>>>> master


# Analyze
def analyze(data):

    SOURCE_PATH, DESTINATION_ARRAY, DESTINATION_TEMP_PATH, DESTINATION_PATH = data

    directory = os.fsencode(SOURCE_PATH)

    for file in os.listdir(directory):

        filename = os.fsdecode(file)

        # Get he order from the filename
        order = int(filename.split('circ')[1].split('_')[0])

<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> master
        # [REMOVE AFTER MAY 25, 2023]
        if order < 13:

            break

<<<<<<< HEAD
=======
>>>>>>> origin/main
>>>>>>> master
        # Get he chromatic_number from the filename
        chromatic_number = int(filename.split(
            '_chi')[1].split('_params.txt')[0])

        # Defien a path to the graph (may not actually exist)
        path = f'{SOURCE_PATH}/{filename}'

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
                RAW_GRAPHS.append(
                    (DESTINATION_TEMP_PATH, DESTINATION_PATH, DESTINATION_ARRAY, order, chromatic_number, string))

            # CLose the file
            f.close()

        except:

            # If the graph path doesn't exist skip
            pass


print('. . . Gathering graph data files from raw graphs folder')
for data in (SOURCE_PATHS):

    analyze(data)


# Before saving
# Create paths if they doesn't exist
print('. . . Creating TEMP directory')
path_manager(TEMP_CRITICAL_P5_CRITICAL_SAVE_PATH,
             TEMP_CRITICAL_P4UP1_CRITICAL_SAVE_PATH)


print('. . . Analyzing graph data')
# For all the raw graphs
for (index, data) in enumerate(RAW_GRAPHS):

<<<<<<< HEAD
=======
<<<<<<< HEAD
    print(f'{index} out of {len(RAW_GRAPHS)}')
=======
>>>>>>> master
    # Progress report
    if (index % int(0.1 * len(RAW_GRAPHS))) == 0:

        print(f'{index} out of {len(RAW_GRAPHS)}')
<<<<<<< HEAD
=======
>>>>>>> origin/main
>>>>>>> master

    # Define the order
    DESTINATION_TEMP_PATH, DESTINATION_PATH, DESTINATION_ARRAY, order, chromatic_number, graph_raw_string = data

    # Remove array props from the graph_raw_string
    graph6_raw_array_to_string = convert_array_to_string(graph_raw_string)

    # graph = circulant(first_elem, remaining_elems)
    E = circulant(order, graph_raw_string)

    # Create graphs and get details
    graph = Graph(E)
    graph6_string = graph.graph6_string()

    # critical_check
    print('. . .  Critical Check')
<<<<<<< HEAD
    is_critical = critical_check(graph, order, chromatic_number)
=======
<<<<<<< HEAD
    is_critical = critical_check(graph, chromatic_number)
=======
    is_critical = critical_check(graph, order, chromatic_number)
>>>>>>> origin/main
>>>>>>> master

    # If this is critical
    if is_critical:

        data = DESTINATION_TEMP_PATH, DESTINATION_PATH, graph6_string, graph6_raw_array_to_string, order, chromatic_number

        # APpend to the DESTINATION_ARRAY
        DESTINATION_ARRAY.append(data)

        # Save the graph
        save(DESTINATION_TEMP_PATH, graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)
<<<<<<< HEAD
        
=======
<<<<<<< HEAD

=======
        
>>>>>>> origin/main
>>>>>>> master
        # print
        print(f'[TEMP] Saving to {DESTINATION_TEMP_PATH}')

# Before saving
# Create paths if they doesn't exist
path_manager(P5_CRITICAL_SAVE_PATH, P4UP1_CRITICAL_SAVE_PATH)

<<<<<<< HEAD
=======
<<<<<<< HEAD
if len(DESTINATION_ARRAY) > 0:

    # Print the generated graphs
    for graph in DESTINATION_ARRAY:

        # Unpack the graph data
        DESTINATION_TEMP_PATH, DESTINATION_PATH, graph6_string, graph6_raw_array_to_string, order, chromatic_number = graph

        # Save the graph
        save(DESTINATION_PATH, graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)
=======
>>>>>>> master
# Print the generated graphs
for graph in DESTINATION_ARRAY:

    # Unpack the graph data
    DESTINATION_TEMP_PATH, DESTINATION_PATH, graph6_string, graph6_raw_array_to_string, order, chromatic_number = graph

    # Save the graph
    save(DESTINATION_PATH, graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)
<<<<<<< HEAD
=======
>>>>>>> origin/main
>>>>>>> master

print(
    f'. . . Success! Saved to {DESTINATION_PATH}')

# Delete the temp folder
shutil.rmtree(TEMP_PATH)
