from sage.all import *
import os
import time
import shutil

# Path to get the data
path = f'../../DATA/graphs'

# Loop through all graphs with 3 - 8 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 8

# Set start time
start_time = time.time()

graphs = []
critical_graphs = []
not_critical = []

# Loop through all graphs with 3 - 7 vertices
for vertice in range(min_vertices, max_vertices + 1):

    # For every grpah in the list
    for chromatic_number in range(min_vertices - 1, max_vertices):

        # Defien the path of the graph
        graph_path = f'{path}/order{vertice}_chi{chromatic_number}.txt'

        try:

            # Open up and read the file graph
            f = open(graph_path, "r")

            # FOr every line in the file
            for line in f:

                # Create a list of nauty_geng connected graphs
                graphs.append(line.split(None, 1)[0])  # add only first word

            # CLose the file
            f.close()

        except:

            pass

# FOr every graph_string in the graphs 
for string in graphs:

    # Create the graph using the string
    graph = Graph(string)
    original_graph = Graph(string)

    # Save the graphs in graph6 format
    graph6_string = original_graph.graph6_string()

    # A flag to check if a graph is critical
    is_critical = True

    # Get its chromatic number
    chromatic_number = original_graph.chromatic_number()

    # Get number of vertices in graph
    number_of_vertices = original_graph.order()

    # Iterate through every vertice in graphs vertices
    for index in range(number_of_vertices):

        # Remove the vertex at index
        graph.delete_vertex(index)

        # Get the new chromatic number
        new_chromatic_number = graph.chromatic_number()

        # Reset the graph to replace deleted vertex
        graph = Graph(original_graph)

        # If the new chromatic number is greater or equal to the current one. Is critical is false
        if new_chromatic_number >= chromatic_number:

            # Is critical is false
            is_critical = False
            
            break
    
    # If the graph is critical
    if is_critical == True:

        # Push it to the critical_graphs list
        critical_graphs.append(string)

# Set end time (not a Prophet tho)
end_time = time.time()

# Calculate the time taken
time_taken = end_time - start_time

print(f'TIme Taken {round(time_taken, 2)}s')
print('Done')

# Analyze the critical graphs
for string in critical_graphs:

    # Create the graph
    graph = Graph(string)

    # Print the analysis
    print(string, ' ', graph.chromatic_number())
