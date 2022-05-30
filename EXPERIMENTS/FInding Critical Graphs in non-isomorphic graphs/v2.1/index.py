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

# Path to get the data
path = f'../../../DATA/graphs'

# Loop through all graphs with 3 - 8 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 9

# Set start time
start_time = time.time()

graphs = []
critical_graphs = []
not_critical = []

def save():

    # Store this graph in the grpahs folder
    f=open(f'./graphs/critical.txt', "a")

    # Store this graph in the grpahs folder
    g=open(f'./graphs/not_critical.txt', "a")

    for graph in critical_graphs:
        
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
        critical_graphs.append(string)

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


# print('critical_graphs: ', critical_graphs)
# print('not_critical: ', not_critical)
