import os


# Define the path manager
def path_manager(*paths):

    for path in paths:

        # Check whether the specified path exists or not
        isExist = os.path.exists(path)

        if not isExist:

            # Create a new directory because it does not exist
            os.makedirs(path)
            print(f"The new directory {path} has been created!")


# Store all the P5_FREE_CIRCULANT_GRAPHS
P5_FREE_CIRCULANT_GRAPHS = []

# The raw and regular graphs
RAW_GRAPHS = []

MAX_VERTICES = 5  # Default=5 || In DATA/raw_graphs you'll find "circ5.x" graphs
max_vertices = 20  # Default=50 || In DATA/raw_graphs you'll find "circ50.x" graphs

print(f'**** STARTING AT {MAX_VERTICES} Vertices ****')

# Create the path for the graphs to be stored
SOURCE_PATH = f'{os.getcwd()}/../DATA/raw graphs'

# Create the path for the graphs to be stored
SAVE_PATH = f'{os.getcwd()}'
DEFAULT_GRAPH6_STRING_SAVE_PATH = f'{SAVE_PATH}/graphs'
DEFAULT_PARAMS_SAVE_PATH = f'{SAVE_PATH}/graphs params'

# Create paths if they doesn't exist
path_manager(DEFAULT_GRAPH6_STRING_SAVE_PATH, DEFAULT_PARAMS_SAVE_PATH)


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
def is_p5_free(graph):
    for v in graph:
        neighbors = graph.neighbors(v)
        for i in range(len(neighbors)):
            for j in range(i+2, len(neighbors)):
                if graph.has_edge(neighbors[i], neighbors[j]):
                    return False

    return True


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
def save(type, string, raw_string, order, chromatic_number):

    if type == 'default':

        # Store this graph in the grpahs folder
        f = open(
            f'{DEFAULT_GRAPH6_STRING_SAVE_PATH}/circ{order}_chi{chromatic_number}.txt', "a")
        e = open(
            f'{DEFAULT_PARAMS_SAVE_PATH}/circ{order}_chi{chromatic_number}_params.txt', "a")

        # Write to file
        f.write(f'{string}\n')
        e.write(f'{raw_string}\n')

        # CLose the file save
        f.close()
        e.close()


print('. . . Gathering graph data files from raw graphs folder')
# Loop through all graphs with {MAX_VERTICES} to {max_vertices}
for order in range(MAX_VERTICES, max_vertices + 1):

    # For every grpah in the list (2 is the minimum number_of_parameters in raw graphs)
    for number_of_parameters in range(max_vertices):

        # Defien a  path to the graph (may not actually exist)
        path = f'{SOURCE_PATH}/circ{order}.{number_of_parameters}.txt'

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

print('. . . Analyzing graph data')
# For all the raw graphs
for (index, data) in enumerate(RAW_GRAPHS):

    # Progress report
    if (index % int(0.1 * len(RAW_GRAPHS))) == 0:

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

    # Is this a p5 free graph
    if is_p5_free(graph):

        # iF so, append it to the P5_FREE_CIRCULANT_GRAPHS
        P5_FREE_CIRCULANT_GRAPHS.append(
            (graph, graph6_string, graph6_raw_array_to_string, order, chromatic_number))

# Print the generated graphs
for graph in P5_FREE_CIRCULANT_GRAPHS:

    # Unpack the graph data
    _, graph6_string, graph6_raw_array_to_string, order, chromatic_number = graph

    # Save the graph
    save('default', graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)


print(
    f'. . . Success! Saved to {DEFAULT_GRAPH6_STRING_SAVE_PATH} and {DEFAULT_PARAMS_SAVE_PATH}')
