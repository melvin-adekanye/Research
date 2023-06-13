import collections
import os
import shutil
import numpy

GRAPH_K = 6
GRPAH6_STRING = []
GRAPH_FILENAME = f"graph-{GRAPH_K}"
PATH_FREE_PATH = "P4+P1 Free" 
SOURCE_PATH = f"../{GRAPH_FILENAME}/{PATH_FREE_PATH}"
LAYOUTS = ["circular", "spring"]
IMAGE_PATHS = []
BASE_IMAGE_PATH = f"Images/{GRAPH_FILENAME}/{PATH_FREE_PATH}"
for LAYOUT in LAYOUTS:
    IMAGE_PATHS.append(f"{BASE_IMAGE_PATH}/{LAYOUT}/")

LEGEND = []

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

path_manager(BASE_IMAGE_PATH, *IMAGE_PATHS)

# Saving graph images to
print(f"Saving graphs to {IMAGE_PATHS}")
for (index, string) in enumerate(GRPAH6_STRING):

    G = Graph(string)

    print(f"{index} - {string}")

    for (image_index, LAYOUT) in enumerate(LAYOUTS):
        p = G.plot(layout=LAYOUT)
        p.save_image(IMAGE_PATHS[image_index] + str(index) + ".png")

    LEGEND.append((index, string))

# Open the file in write mode
with open(f'{BASE_IMAGE_PATH}/Image Legend.txt', 'w') as file:

    # Perform the loop and write to the file
    for i in LEGEND:
        index, string = i
        line = f"{index} => {string} \n"
        file.write(line)

print("Completed ⭐")
