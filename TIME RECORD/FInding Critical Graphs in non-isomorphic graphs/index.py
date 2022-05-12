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

        # Assign the original graph
        original_graph = Graph(graph)

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
        
        # A quick print out
        # print(f"is '{graph6_string}' critical? {is_critical}")

        # Save. (Only if graph is critical)
        if is_critical:

            # Store this graph in the grpahs folder
            f=open(f'./graphs/order{vertice}_chi{chromatic_number}.txt', "a")

            # Write to file
            f.write(f'{graph6_string}\n')

            # CLose the file
            f.close()

# Set end time (not a Prophet tho)
end_time=time.time()

# Calculate the time taken
time_taken=end_time - start_time

print(f'TIme Taken {round(time_taken, 2)}s')
print('Done')
