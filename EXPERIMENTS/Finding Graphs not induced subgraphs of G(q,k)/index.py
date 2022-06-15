from sage.all import *
import os
import time
import shutil

# Create the path for the graphs to be stored
# path = f'{os.getcwd()}/graphs'

# Remove graphs
# shutil.rmtree(pth, ignore_errors=True)

# Wait a bit
# time.sleep(3)

# Then create a new folder
# os.mkdir(path)

# The following is a generalizing the Gr and Gp constructions for 4- and 5-crit graphs, resp. to create
# an infinite family of (k+1)-crit graphs for each k>=2

def Gq(q, k):

    L = []

    for i in range(0, k*q+1):

        e1 = {i, (i+1) % (k*q+1)}

        e2 = {i, (i-1) % (k*q+1)}

        if e1 not in L:

            L.append(e1)

        if e2 not in L:

            L.append(e2)

        for j in range(0, q):

            for m in range(2, k):

                e1 = {i, (i+k*j+m) % (k*q+1)}

                if e1 not in L:

                    L.append(e1)

    return Graph(L)


# Path to get the data
path = f'../../DATA/graphs'

# Loop through all graphs with 3 - 9 vertices
min_vertices = 3
max_vertices = 9

starting_q = 1
ending_q = 10

# 3, 4, 5, 6
k_value = 5

print(f'starrting k = {k_value}')

# Set start time
start_time = time.time()

graphs = []
not_induced_subgraphs = []

# Read in the graphs
# Loop through all graphs with min_vertices to max_vertices vertices
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
for index, string in enumerate(graphs):

    # Is a sub graph
    graph_is_a_subgraph = False

    # For every graph in Gq()
    # Search for these graphs are not induced subgraphs of G(q,k) for all q
    for q in range(starting_q, ending_q + 1):

        # Define the Gq graph
        Gq_graph = Gq(q, k_value)

        # Create the graph using the string
        graph = Graph(string)
        original_graph = Graph(string)

        # CHeck if graph is a subgraph Gq_graph
        graph_is_a_subgraph = graph.is_subgraph(Gq_graph, induced=True)

        # If graph is found to be a sub graph
        if graph_is_a_subgraph == True:
            
            # Stop
            break

    # If this is not a subgraph
    if graph_is_a_subgraph == False:

        # Append teh string to the not_induced_subgraphs
        not_induced_subgraphs.append(string)

    # Progress report
    if index % 1000 == 0:
        print(f'Graph {index} of {len(graphs)}')


# Set end time (not a Prophet tho)
end_time = time.time()

# Calculate the time taken
time_taken = end_time - start_time

# Save to file

# Store this graph in the grpahs folder
f = open(f'./graphs/not induced subgraphs of k = {k_value}.txt', "a")

for string in not_induced_subgraphs:

    # Write to file
    f.write(f'{string}\n')

# CLose the file
f.close()

print(f'TIme Taken {round(time_taken, 2)}s')
print(f'Done where k = {k_value}')

