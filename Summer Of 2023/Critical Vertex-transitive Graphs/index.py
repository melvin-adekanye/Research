import os
import shutil
from sage import *

# Get the source of the data
SOURCE_PATH = f'{os.getcwd()}/DATA'

# graph_path =
GRAPH6_STRINGS = []
CRITICAL_GRAPH6_STRINGS = []


# Critical check. Takes in the graph6_string, graph raw string and the chromatic number
def critical_check(graph, chromatic_number):

    # if there is no cn
    if chromatic_number == None:

        # Get it from the graph
        chromatic_number = graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == k:

        # Get the order of the graph
        order = graph.order()

        # A flag to check if a graph is critical
        is_critical = True

        # First node
        first_node = 0

        # print out the vertex being deleted
        print(f'. . . Deleting Vertice-{first_node} of {order}')

        # Remove the vertex at index
        graph.delete_vertex(first_node)

        # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
        if vertex_coloring(graph, k=chromatic_number-1, value_only=True) == False:

            # Is critical is false
            is_critical = False


        # Return is_critical
        return is_critical

    # Return not critical to k value
    return False


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
for folder in os.listdir(directory):

    sub_folder = os.fsdecode(folder)

    # Open the sub folder in DATA
    sub_path = f'{SOURCE_PATH}/{sub_folder}'

    sub_directory = os.fsencode(sub_path)
    for file in os.listdir(sub_directory):

        filename = os.fsdecode(file)

        # Try: If the GRAPH_PATH does exist
        try:

            # Open up and read the file graph
            f = open(sub_path, "r")

            # FOr every line in the file
            for line in f:

                # Attain the raw graph numbers
                string = line.split(None)[0]

                # Append a list to the raw graph
                GRAPH6_STRINGS.append((string, filename))

            # CLose the file
            f.close()

        except:

            # If the graph path doesn't exist skip
            pass


print('. . . Analyzing graph data')
# Loop through all the GRPAH6_STRING
for (index, data) in enumerate(GRAPH6_STRINGS):

    # Get the graph6 string and file name
    graph6_string, filename = data

    # Progress report
    if (index % int(0.1 * len(GRAPH6_STRINGS))) == 0:

        print(
            f'{index} out of {len(GRAPH6_STRINGS)} Graphs')

    # Create graphs and get details
    graph = Graph(graph6_string)
    chromatic_number = graph.chromatic_number()

    # Check if this graph is critical
    is_critical = critical_check(graph, chromatic_number)

    # If this graph is critical
    if is_critical == True:

        CRITICAL_GRAPH6_STRINGS.append(graph6_string)