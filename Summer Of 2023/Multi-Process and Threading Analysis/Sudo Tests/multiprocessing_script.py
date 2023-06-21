import networkx as nx
import matplotlib.pyplot as plt
from itertools import combinations
import datetime
import os
import copy
from concurrent.futures import ThreadPoolExecutor
import multiprocessing
from sage.graphs.graph_coloring import vertex_coloring
import random

SOURCE_PATH = f'./raw graphs'

# K = chromatic number
GRAPH_K = 5

min_vertices = 5  # 5  # circ-5 graphs
max_vertices = 20  # 20  # circ-50 graphs

writeToFile = True

# This is the current graph file index
currentGraphFile = 0

# This is how many G6 strings go into each file
graphFileSize = 500

foundGraphs = 0

# This is the array of Graph6 outputs
outputLineArray = []

# This is the current output stream
outputStream = None

# For a memory cost, we can use this to avoid doing the checks twice for subgraphs when we remove nodes
criticalCache = {}

# This is to test the equality between the Graph6 keys and the str() function keys
testStrCache = {}

# We can also cache the Graph6 strings
graph6Cache = {}

# This is where the g6 files will be stored
DESTINATION_PATH = "critical graphs"


def critical_check(graph):

    # Set the original graph before messing with it
    original_graph = Graph(graph)

    # Get it from the graph
    chromatic_number = original_graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == GRAPH_K:

        # Get the order of the graph
        order = original_graph.order()

        # A flag to check if a graph is critical
        is_critical = True

        # Iterate through every vertice in graphs vertices
        for index in range(order):

            # print out the vertex being deleted
            print(f'. . . Deleting Vertice-{index} of {order}')

            # Remove the vertex at index
            graph.delete_vertex(index)

            # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
            if vertex_coloring(graph, k=GRAPH_K-1, value_only=True) == False:

                # Is critical is false
                is_critical = False

                # Not critical, lets break the loop
                break

        # Reset the graph to replace deleted vertex
        graph = Graph(original_graph)

        # Return is_critical
        return is_critical

    # Return not critical to k value
    return False


def multiprocess_critical_check(original_graph_and_index, chromatic_number, flag):

    if flag.value:

        return

    graph, index = original_graph_and_index
    print(f". . . . . . Recieved {graph.graph6_string()} - {index}")

    # Only if the CN is K
    if chromatic_number == GRAPH_K:

        print(f". . . . . . Deleting node {index}")

        # Remove the vertex at index
        graph.delete_vertex(index)

        # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
        if not vertex_coloring(graph, k=GRAPH_K-1, value_only=True):

            print(f". . . . . . NOT Critical")

            flag.value = False

        flag.value = True

    return flag.value


def multiprocess_critical_check_manager(graph):

    print(f". . . Analyzing {graph.graph6_string()}")

    original_graphs = []

    processes = []

    is_critical = False

    # Get it from the graph
    chromatic_number = graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == GRAPH_K:

        for i in range(graph.order()):

            original_graphs.append([Graph(graph), i])

        # Create a shared flag variable
        flag = multiprocessing.Value('b', False)

        is_critical = True

        # For every node in the original_graph create a process
        for original_graph_and_index in original_graphs:

            process = multiprocessing.Process(target=multiprocess_critical_check, args=(
                original_graph_and_index, chromatic_number, flag))
            process.start()
            processes.append(process)

            is_critical = bool(flag.value)

            if is_critical == False:

                # Wait for all processes to complete
                for process in processes:
                    process.join()

                break

        print(is_critical)
    
    return is_critical


def saveFiles(writeToFile):

    print(f"{len(outputLineArray)} graphs found!")

    if not writeToFile:

        print("DId not save to ./critical graphs")
        return

    # Open up the file path
    path = os.path.join(DESTINATION_PATH, f"graph_{GRAPH_K}.g6")
    open(path, 'w').close()

    outputStream = open(path, "w")

    temp_output_stream = ""

    for graph6_string in outputLineArray:

        temp_output_stream += graph6_string
        temp_output_stream += "\n"

    outputStream.write(temp_output_stream)

    # CLose up shop
    outputStream.close()


def multithreading():

    print("multiFoo() is RUNNING")

    for (index, graph) in enumerate(GRPAH6_STRING):

        graph = Graph(graph)

        print(index, f" out of {len(GRPAH6_STRING)} ", graph.graph6_string())

        # Create the graph_string
        graph_string = graph.graph6_string()

        # is_critical = critical_check(graph) #multiprocess_critical_check_manager(graph)
        is_critical = multiprocess_critical_check_manager(graph)

        print(f"main(): {is_critical}")

        # CHeck if the graph is critical
        if is_critical == True:
            # if critical_check(graph):

            outputLineArray.append(graph_string)

    # Save to file
    saveFiles(True)

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

# Attain the graph string from circulant graph


def circulant(n, L):

    E = []

    for i in range(n):

        for j in range(i+1, n):

            if (((i-j) % n) in L):

                if ({i, j} not in E):

                    E.append({i, j})

    # Return the graph
    return E


if __name__ == '__main__':

    print("Prepping!")
    # If DESTINATION_PATH doesn't exist
    if (not os.path.exists(DESTINATION_PATH)):

        # Create it
        os.makedirs(DESTINATION_PATH)

    raw_graphs = []
    GRPAH6_STRING = []

    """
    # Read in the 5-critical graphs
    print('. . . Gathering graph data files from raw graphs folder')
    # Loop through all graphs with {min_vertices} to {max_vertices}
    for order in range(min_vertices, max_vertices + 1):

        # For every grpah in the list (2 is the minimum chromatic number in raw graphs)
        for circulant_number in range(max_vertices):

            # Defien a  path to the graph (may not actually exist)
            path = f'{SOURCE_PATH}/circ{order}.{circulant_number}.txt'

            # Try: If the graph_path does exist
            try:

                # Open up and read the file graph
                f = open(path, "r")

                # FOr every line in the file
                for line in f:

                    # Attain the raw graph numbers
                    string = line.split(None)

                    string = list(map(int, string))

                    # Append a list to the raw graph
                    raw_graphs.append([order, string])

                # CLose the file
                f.close()

            except:

                # If the graph path doesn't exist skip
                pass

    print('. . . Analyzing graph data')
    # For all the raw graphs
    for (index, data) in enumerate(raw_graphs):

        if index % int(len(raw_graphs) / 10) == 0:
            print(index, "of", len(raw_graphs))

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

        GRPAH6_STRING.append(graph6_string)


    with open('GRPAH6_STRING.txt', 'w') as file:
        # Write each item from the list to a new line in the file
        for item in GRPAH6_STRING:
            file.write(item + '\n')

    """

    # Open up and read the file graph
    f = open("GRPAH6_STRING.txt", "r")

    # FOr every line in the file
    for line in f:

        # Attain the raw graph numbers
        string = line.split(None)[0]

        # Append a list to the raw graph
        GRPAH6_STRING.append(string)

    start_time = datetime.datetime.now()

    print("Starting!")

    multithreading()

    end_time = datetime.datetime.now()

    elapsed_time = end_time - start_time
    totalSeconds = elapsed_time.total_seconds()

    hours, remainder = divmod(int(totalSeconds), 3600)
    minutes, seconds = divmod(remainder, 60)
    roundSeconds = float("{:.2f}".format(math.modf(totalSeconds)[0] + seconds))

    print("Elapsed time: {} hours, {} minutes, {} seconds".format(
        int(hours), int(minutes), roundSeconds))
