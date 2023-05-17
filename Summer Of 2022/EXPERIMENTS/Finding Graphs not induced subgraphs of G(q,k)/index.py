from sage.all import *
import os
import time
import shutil

# Loop through all graphs with 3 - 9 vertices
min_vertices = 3  # 3
max_vertices = 9  # 9

starting_q = 1  # 1
ending_q = 10  # 10

# 3, 4, 5, 6
k_value = int(input(f'k='))

# Store all read data_path graphs
graphs = []

# Path to get the data
data_path = f'../../DATA/graphs'

# Defien the file path
file_path = f'{os.getcwd()}/not induced subgraphs of k={k_value}'

# Ask to delete file path
x = input(f'Delete {file_path} (y/n): ')

# if yes. Delete and create 
if x == 'y':

    # Remove graphs
    shutil.rmtree(file_path, ignore_errors=True)

    # Wait a bit
    time.sleep(3)

    # Then create a new folder
    os.mkdir(file_path)

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

# Set start time
start_time = time.time()

print(f'starrting k = {k_value}')

# Save to file
def save_graph(string, chromatic_number, vertice):

    # Store this graph in the grpahs folder
    graph_path = open(f'{file_path}/order{vertice}_chi{chromatic_number}.txt', 'a')

    # Write to file
    graph_path.write(f'{string}\n')

    # CLose the file
    graph_path.close()


# Read in the graphs
# Loop through all graphs with min_vertices to max_vertices vertices
for vertice in range(min_vertices, max_vertices + 1):

    # For every grpah in the list
    for chromatic_number in range(min_vertices - 1, max_vertices):

        # Defien the path of the graph
        graph_path = f'{data_path}/order{vertice}_chi{chromatic_number}.txt'

        try:

            # Open up and read the file graph
            f = open(graph_path, "r")

            # FOr every line in the file
            for line in f:

                # Create a list of nauty_geng connected graphs
                # add only first word
                graphs.append([line.split(None, 1)[0], chromatic_number, vertice])

            # CLose the file
            f.close()

        except:

            pass

# FOr every graph_string in the graphs
for index, graph in enumerate(graphs):

    string = graph[0]
    chromatic_number = graph[1]
    vertice = graph[2]

    H = Graph(string)  # graph to test if induced subgraph

    in_sub = False  # flag that will switch to True if subgraph of some value of q

    for q in range(starting_q, ending_q + 1):

        G = Gq(q, k_value)

        # subgraph search is used as we need isomorphic subgraphs not exact subgraphs
        sub_search = G.subgraph_search(H, induced=True)

        # the H is not an induced subgraph of G if and only if the previous line returns None
        if sub_search is not None:

            in_sub = True

            break

    if in_sub == False:

        save_graph(string, chromatic_number, vertice)

    # Progress report
    if index % 100 == 0:
        print(f'Graph {index} of {len(graphs)}')

# Set end time (not a Prophet tho)
end_time = time.time()

# Calculate the time taken
time_taken = end_time - start_time



print(f'TIme Taken {round(time_taken, 2)}s')
print(f'Done where k = {k_value}')
