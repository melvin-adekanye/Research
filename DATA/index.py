from sage.all import *
import os
import time
import shutil

# Create the path for the graphs to be stored
path = f'{os.getcwd()}/graphs'

# Remove graphs
shutil.rmtree(path, ignore_errors=True)

# Wait a bit
time.sleep(3)

# Then create a new folder
os.mkdir(path)

# Loop through all graphs with 3 - 8 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 8

# Set start time
start_time = time.time()

# Loop through all graphs with 3 - 7 vertices
for vertice in range(min_vertices, max_vertices + 1):

    # Create the query
    query = "%d -c" % (vertice)

    # Create a list of nauty_geng connected graphs
    graph_list = list(
        graphs.nauty_geng(query)
    )

    # For every grpah in the list
    for graph in graph_list:

        # Save the graphs in graph6 format
        graph6_string = graph.graph6_string()

        # Get its chromatic number
        chromatic_number = graph.chromatic_number()

        # Store this graph in the grpahs folder
        f = open(f'./graphs/order{vertice}_chi{chromatic_number}.txt', "a")

        # Write to file
        f.write(f'{graph6_string}\n')

        # CLose the file
        f.close()

# Set end time (not a Prophet tho)
end_time = time.time()

# Calculate the time taken
time_taken = end_time - start_time

print(f'TIme Taken {round(time_taken, 2)}s')
print('Done')
