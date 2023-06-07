import networkx as nx
import matplotlib.pyplot as plt
from itertools import combinations
import datetime
import os
import copy
from concurrent.futures import ThreadPoolExecutor
import multiprocessing
from sage.graphs.graph_coloring import vertex_coloring

SOURCE_PATH = f'./raw graphs'

# K = chromatic number
GRAPH_K = 5

min_vertices = 20 # 5  # circ-5 graphs
max_vertices = 30 # 20  # circ-50 graphs

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


def has_valid_coloring(adjacency_dict):

    # graph6DictString = graph_to_graph6_3(adjacency_dict)
    STRdictString = str(adjacency_dict)

    # getter = criticalCache.get(graph6DictString, None)
    getterStr = testStrCache.get(STRdictString, None)

    if (getterStr != None):
        # print(criticalCache[graph6DictString] == testStrCache[STRdictString])
        return getterStr

    # Determine the maximum degree of any node in the graph
    node_degrees = [len(neighbors) for neighbors in adjacency_dict.values()]

    if (len(node_degrees) == 0):
        cache[dictString] = 0
        return 0

    max_degree = max(node_degrees)

    # Define a function that recursively tries all possible colorings
    def try_coloring(coloring, node_order):
        # If all nodes have been colored, check if the coloring is valid
        if len(coloring) == len(adjacency_dict):
            for node, neighbors in adjacency_dict.items():
                for neighbor in neighbors:

                    # Normally this check isn't needed, but we need it when we remove nodes from the dictionary
                    # Of course, we need to remove nodes to check if a graph is critical
                    dictNode = coloring.get(node, None)
                    dictNeighbor = coloring.get(neighbor, None)

                    if (dictNode != None and dictNeighbor != None):
                        if dictNode == dictNeighbor:
                            return 0

            max_color = max(coloring[node] for node in adjacency_dict.keys())

            # Return the maximum color
            return max_color

        # Otherwise, recursively try all possible color assignments for the next node
        next_node = node_order[len(coloring)]
        neighborhood = adjacency_dict[next_node]

        for color in range(max_degree + 1):
            if all(color != coloring.get(neighbor, None) for neighbor in neighborhood):
                coloring[next_node] = color

                max_color = try_coloring(coloring, node_order)

                if (max_color != 0):
                    # Return the maximum color
                    return max_color
        return 0

    # Generate a list of nodes ordered by degree (highest first)
    nodes = list(adjacency_dict.keys())
    node_order = sorted(nodes, key=lambda node: -len(adjacency_dict[node]))

    # Try all possible colorings starting with an empty coloring
    colors = list(range(max_degree + 1))
    chi = try_coloring({}, node_order)
    # criticalCache[graph6DictString] = chi
    testStrCache[STRdictString] = chi
    # print(cache[dictString])
    return chi + 1


def adjacencyDict(edges):
    # Build a dictionary where each edge is a key and its value is a set of neighboring edges
    adjacency_dict = {}
    for edge in edges:
        if edge[0] not in adjacency_dict:
            adjacency_dict[edge[0]] = set()
        if edge[1] not in adjacency_dict:
            adjacency_dict[edge[1]] = set()
        adjacency_dict[edge[0]].add(edge[1])
        adjacency_dict[edge[1]].add(edge[0])

    # print(adjacency_dict)
    return adjacency_dict


def remove_node(adj_dict, node):
    # create a copy of the adjacency dictionary to modify
    new_adj_dict = copy.deepcopy(adj_dict)

    # remove the node and its edges from the dictionary

    # This takes care of the node
    del new_adj_dict[node]

    # This loops through the edges
    for neighbor in adj_dict[node]:

        getter = new_adj_dict.get(neighbor, None)
        if (getter != None):
            if (node in new_adj_dict[neighbor]):
                new_adj_dict[neighbor].remove(node)
    # del new_adj_dict[node]

  # shift down the node indices higher than the removed node
    for i in range(node+1, max(new_adj_dict)+1):
        if i in new_adj_dict:
            new_adj_dict[i-1] = new_adj_dict.pop(i)
            new_adj_dict[i-1] = {j-1 if j >
                                 node else j for j in new_adj_dict[i-1]}

    return new_adj_dict


def dict_after_remove_edge(edges, edgeToRemove):
    newEdges = edges.copy()
    newEdges.remove(edgeToRemove)
    return adjacencyDict(newEdges)

# This returns whether an edge can be deleted while still keeping the chromatic number the same


def isCriticalOnK(adjacency_dict, edges, k):

    # We can't have a k-critical graph when there's fewer than k nodes
    # However, we can have a critical graph with k or more nodes depending on where the edges are
    if (len(adjacency_dict) < k):
        return False

    # A k-critical graph has exactly k as its chromatic number
    chi = has_valid_coloring(adjacency_dict)
    if (chi != k):
        return False

    # For any node that we try to remove, the graph is critical when it loses chromatic number
    for edge in edges:

        # Graphs with isolated vertices are never critical.
        # You can color an isolated vertex whatever you want
        # if (len(adjacency_dict[node]) == 0):
        # print(adjacency_dict)
        # return False

        temp_dict = dict_after_remove_edge(edges, edge)
        chiCrit = has_valid_coloring(temp_dict)

        # If the chromatic number stays the same for a node we remove, the graph is not critical
        # Every node must be necessary to keep chi where it is.
        # If there's nodes that aren't strictly necessary, we're hooped
        if (chiCrit == k):
            return False

    # The graph has passed all the tests
    return True


def graph_to_graph6_3(adj_dict):

    dictString = str(adj_dict)

    # This cache can get big, so we only do it for small graphs
    if (len(adj_dict) <= 5):
        getter = graph6Cache.get(dictString, None)
        if (getter != None):
            return getter

    # Create a graph object from the adjacency dictionary
    G = nx.Graph(adj_dict)

    # Obtain a Graph6 string from the graph object
    graph6 = nx.to_graph6_bytes(G).decode('ascii')

    # Remove the newline character at the end of the string
    graph6 = graph6.rstrip('\n')

    if (len(adj_dict) <= 5):
        graph6Cache[dictString] = graph6

    return graph6


def removeSubstring(myStr, substring):
    output_string = ""
    str_list = myStr.split(substring)
    for element in str_list:
        output_string += element
    return output_string


def subdirectory(subdirectoryName, number):
    # Get the current directory path
    current_directory = os.path.dirname(os.path.abspath(__file__))

    # Create the subdirectory if it doesn't exist
    subdirectory_path = os.path.join(current_directory, subdirectoryName)
    os.makedirs(subdirectory_path, exist_ok=True)

    # Create the file within the subdirectory
    file_path = os.path.join(
        subdirectory_path, "classified graphs " + str(number) + ".txt")
    return file_path


def showGraph(dict_):
    # print(dict_)

    G = nx.Graph(dict_)

    color_array = ["red", "blue", "green", "gold", "orange", "purple", "teal",
                   "black", "grey", "steelblue", "magenta", "violet", "dodgerblue", "brown"]
    color_array_trunc = []

    for i in range(len(dict_)):
        color_array_trunc.append(color_array[i])

    # Create a dictionary that maps each node to its color
    # color_map = {node: color_array[coloring[node]] for node in adjacency_dict.keys()}

    # Draw the colored graph using networkx
    nx.draw(G, with_labels=True, node_color=color_array_trunc, font_color='w')
    plt.show()


def classifyGraph(adjacencyDict, edges, minK, maxK):

    # We're looking for
    for i in range(minK, maxK + 1):

        if (isCriticalOnK(adjacencyDict, edges, i)):
            # print("Critical graph found!")
            return [True, "The graph is critical on: " + str(i)]

    return [False, "The graph is not critical in the k range of " + str(minK) + " - " + str(maxK)]


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
            if vertex_coloring(graph, k=k-1, value_only=True) == False:

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


def cc_mp(original_graphs):

    graph, index = original_graphs

    # Get it from the graph
    chromatic_number = graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == GRAPH_K:

        # Remove the vertex at index
        graph.delete_vertex(index)

        # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
        if not vertex_coloring(graph, k=GRAPH_K-1, value_only=True):

            return False

        return True

    return False


def critical_check_mp(graph, pool):

    original_graphs = []

    is_critical = False

    # Set the original graph before messing with it
    original_graph = Graph(graph)

    # Get it from the graph
    chromatic_number = original_graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == GRAPH_K:

        for i in range(original_graph.order()):

            # Set the original graph before messing with it
            new_original_graph = Graph(graph)

            original_graphs.append([new_original_graph, i])

        result = pool.map(cc_mp, original_graphs)

        if False not in result:

            is_critical = True


    return is_critical


def foo(G, pool):

    # Create the graph_string
    graph_string = G.graph6_string()

    # CHeck if the graph is critical
    if (critical_check_mp(G, pool)):
    # if critical_check(G):

        outputLineArray.append(graph_string)




def multiFoo(graphArrays):

    # Create a thread pool executor with the specified number of threads
    # pool = multiprocessing.Pool()
    print("multiprocessing Pool Initialized")
    pool = multiprocessing.Pool()

    for (index, arr) in enumerate(graphArrays):
        print(index, f" out of {len(graphArrays)} ", arr.graph6_string())

        foo(arr, pool)
        # pool.map(foo, arr)

    # No more swimming 🌊
    # pool.close()
    # pool.join()
    pool.close()
    pool.join()


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


def get_sublists(original_list, number_of_sub_list_wanted):
    sublists = list()
    for sub_list_count in range(number_of_sub_list_wanted):
        sublists.append(
            original_list[sub_list_count::number_of_sub_list_wanted])
    return sublists


def multithreading(all_graphs):

    # print("multiprocessing is ON")
    # Create a thread pool executor with the specified number of threads
    # pool = multiprocessing.Pool()

    # print(f"Subdividing {num_of_graphs} graphs into {num_threads}")
    # test = get_sublists(all_graphs, num_threads)

    print("multiFoo() is RUNNING")
    multiFoo(all_graphs)

    # Get the results from each future as they become available
    # pool.map(multiFoo, test)

    # No more swimming 🌊
    # pool.close()
    # pool.join()/

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

            if(((i-j) % n) in L):

                if({i, j} not in E):

                    E.append({i, j})

    # Return the graph
    return E


print("Prepping!")
# If DESTINATION_PATH doesn't exist
if (not os.path.exists(DESTINATION_PATH)):

    # Create it
    os.makedirs(DESTINATION_PATH)

raw_graphs = []
GRPAH6_STRING = []

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

    GRPAH6_STRING.append(graph)



start_time = datetime.datetime.now()

print("Starting!")

multithreading(GRPAH6_STRING)

end_time = datetime.datetime.now()

elapsed_time = end_time - start_time
totalSeconds = elapsed_time.total_seconds()

hours, remainder = divmod(int(totalSeconds), 3600)
minutes, seconds = divmod(remainder, 60)
roundSeconds = float("{:.2f}".format(math.modf(totalSeconds)[0] + seconds))

print("Elapsed time: {} hours, {} minutes, {} seconds".format(
    int(hours), int(minutes), roundSeconds))
