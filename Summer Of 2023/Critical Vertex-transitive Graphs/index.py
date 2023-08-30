import os
import shutil
from sage import *
from sage.graphs.graph_coloring import vertex_coloring

# Get the source of the data
SOURCE_PATH = f'{os.getcwd()}/DATA'

# graph_path =
GRAPH6_STRINGS = []
CRITICAL_GRAPH6_STRINGS = []

K_CRITICAL = 5
K_CRITICAL_FILENAME = f"{K_CRITICAL}-critical"

SAVE_PATH = f'{os.getcwd()}/{K_CRITICAL_FILENAME}'
TEMP_PATH = f'{os.getcwd()}/temp'
TEMP_SAVE_PATH = f"{TEMP_PATH}/{K_CRITICAL_FILENAME}"

print(
    f'****  Finding {K_CRITICAL}-Critical Vertex-transitive Graphs ****')

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


print('. . . Creating TEMP directory')
path_manager(TEMP_PATH, TEMP_SAVE_PATH)


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
def critical_check(graph, chromatic_number):

    # if there is no cn 
    if chromatic_number == None:

        # Get it from the graph
        print("Error Occured with CN")
        return False

    # A flag to check if a graph is critical
    is_critical = True

    # First node
    first_node = 0

    # Remove the vertex at index
    graph.delete_vertex(first_node)

    # Determinging vertex critical
    print(". . . Determining Vertex Critical")
    # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
    if vertex_coloring(graph, k=chromatic_number-1, value_only=True) == False:

        # Is critical is false
        is_critical = False

    print(". . . . . . Found!! Vertex Critical")

    # Return is_critical
    return is_critical
<<<<<<< HEAD
=======
>>>>>>> origin/main
>>>>>>> master


# The save function
def save(SAVE_PATH, string, filename):

    try:

        DEFAULT_GRAPH6_STRING_SAVE_PATH = f'{SAVE_PATH}'

        # Store this graph in the grpahs folder
        f = open(
            f'{DEFAULT_GRAPH6_STRING_SAVE_PATH}/{filename}.g6', "a+")

        # Write to file
        f.write(f'{string}\n')

        # CLose the file save
        f.close()

    except:

        print("Create folder '/critical' if it doesn't already exist!")


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
            f = open(sub_path + "/" + filename, "r")

            # FOr every line in the file
            for line in f:

                # Attain the raw graph numbers
                string = line.split(None)[0]

                # Append a list to the raw graph
                GRAPH6_STRINGS.append((string, filename))

            # CLose the file
            f.close()

        except e:

            print(e)

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

    # Calc the chromatic number
    chromatic_number = graph.chromatic_number()

    if chromatic_number == K_CRITICAL:
        
        print(f"{graph6_string} - Critical Check")

        # Check if this graph is critical
        is_critical = critical_check(graph, chromatic_number)

        # If this graph is critical
        if is_critical == True:

            CRITICAL_GRAPH6_STRINGS.append((graph6_string, filename))

            # Save the graph
            save(TEMP_SAVE_PATH, graph6_string, filename)


# Print the generated graphs
for graph in CRITICAL_GRAPH6_STRINGS:

    # Unpack the graph data
    graph6_string, filename = graph

    # Save the graph
    save(SAVE_PATH, graph6_string, filename)


# Delete the temp folder
shutil.rmtree(TEMP_PATH)
