
from sage.all import *
import os
import time
import shutil

# Create the path for the graphs to be stored
path = f'{os.getcwd()}/critical_graphs'
path2 = f'{os.getcwd()}/not_critical_graphs'

# Remove graphs
shutil.rmtree(path, ignore_errors=True)
shutil.rmtree(path2, ignore_errors=True)

# Wait a bit
time.sleep(3)

# Then create a new folder
os.mkdir(path)
os.mkdir(path2)

# Loop through all graphs with 3 - 9 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 10

# Set start time
start_time = time.time()

graph_list = []
critical_graphs = []
not_critical = []
vertices = []

def save():

    for graph in critical_graphs:
        
        # Store this graph in the grpahs folder
        f = open(f'./critical_graphs/order{vertice}_chi{chromatic_number}.txt', "a")

        # Save the graphs in graph6 format
        graph6_string = graph.graph6_string()

        # Get its chromatic number
        chromatic_number = graph.chromatic_number()

        # Write to file
        f.write(f'{graph6_string}\n')

        # CLose the file
        f.close()
        
        
        # Write to file
        f.write(f'{graph}\n')

    for graph in not_critical:
        
        # Write to file
        g.write(f'{graph}\n')


    # CLose the file
    f.close()
    g.close()

# Loop through all graphs with 3 - 7 vertices
for vertice in range(min_vertices, max_vertices + 1):

    # Create the query
    query = "%d -c" % (vertice)

    # Create a list of nauty_geng connected graphs
    graph_list = list(
        graphs.nauty_geng(query)
    )

    # Append the vertice to the vertices
    vertices.append(vertice)

# Merge the vertices along with the 
graph_list = [[x,y,z] for x,y,z in zip(A,B,C)]

# FOr every graph_string in the graphs
for string in graph_list:

    # Create the graph using the string
    graph = Graph(string)
    original_graph = Graph(string)

    # A flag to check if a graph is critical
    is_critical = True

    # Get its chromatic number
    chromatic_number = original_graph.chromatic_number()

    # Degrees
    degrees = original_graph.degree()

    # Create the degrees and index
    indice_n_degree = [[index, degree]
                       for index, degree in enumerate(degrees)]

    # Delete highest degree vertice first
    delete_vertice_with_highest_degree_first = False

    # Sort the indice_n_degree by the degree value
    indice_n_degree.sort(
        key=lambda x: x[1], reverse=delete_vertice_with_highest_degree_first)

    # Define min degree
    min_degree = indice_n_degree[0][0]
    
    # Check if the minimum degree is smaller than k - 1. If so, it is not critical
    if min_degree > (chromatic_number - 1):
        
        # Iterate through every vertice in graphs vertices
        for (index, degree) in indice_n_degree:

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
        critical_graphs.append([string, chromatic_number, ])

    else:

        not_critical.append(string)


# Set end time (not a Prophet tho)
end_time = time.time()

# Calculate the time taken
time_taken = end_time - start_time

# Time taken print out
print(f'TIme Taken {round(time_taken, 2)}s')
print('Done')

save()

