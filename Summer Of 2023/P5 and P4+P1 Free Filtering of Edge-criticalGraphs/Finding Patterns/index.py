import collections
import os
import shutil
import numpy

GRAPH_K = 5
GRPAH6_STRING = []
SOURCE_PATH = f"../graph-{GRAPH_K}/" + "P4+P1 Free"
LAYOUT = "circular"
IMAGE_PATH = f"images/{LAYOUT}/"


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


# Reading in the files from {SOURCE_PATH}
print(f"Reading in the files from {SOURCE_PATH}")
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
            GRPAH6_STRING.append(string)

        # CLose the file
        f.close()

    except:

        # If the graph path doesn't exist skip
        pass

    print(GRPAH6_STRING)

path_manager(IMAGE_PATH)

def is_C5_free(graph):

    c5 = graphs.CycleGraph(5)

    return graph.subgraph_search(c5, induced=True) == None


# Saving graph images to
print(f"Saving graphs to {IMAGE_PATH}")
for (index, string) in enumerate(GRPAH6_STRING):

    G = Graph(string)

    if is_C5_free(G) == True:

        print(index, " ", string, " C5 Free")
        
    
    # print(G.distance_matrix())
    # print("===============")
    # print(G.adjacency_matrix())
    # print(f"\n\n{string}")


    p = G.plot(layout=LAYOUT)
    p.save_image(IMAGE_PATH + str(index) + ".png")

print("Completed ⭐")
