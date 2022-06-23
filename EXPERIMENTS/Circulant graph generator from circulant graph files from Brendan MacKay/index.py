import os
import time
import shutil

from sage.graphs.graph_coloring import vertex_coloring

x = input('delete "/critical graphs" and "/graphs" (y/n): ')

if x == 'y':

    # Create the path for the graphs to be stored
    raw_path = f'{os.getcwd()}/raw graphs'
    path = f'{os.getcwd()}/graphs'
    critical_path = f'{os.getcwd()}/critical graphs'

    # Remove graphs
    shutil.rmtree(path, ignore_errors=True)
    shutil.rmtree(critical_path, ignore_errors=True)

    # Wait a bit
    time.sleep(3)

    # Then create a new folder
    os.mkdir(path)
    os.mkdir(critical_path)


raw_graphs = []
graphs = []

min_vertices = 5 # circ-5 graphs
max_vertices = 30 # circ-50 graphs

k = 5

def critical_check(string):

    # Create the graph using the string
    graph = Graph(string)
    original_graph = Graph(string)

    # A flag to check if a graph is critical
    is_critical = True

    # Degrees
    degrees = original_graph.degree()
    order = original_graph.order()

    # Iterate through every vertice in graphs vertices
    for index in range(order):

        print(f'. . . Deleting Vertice-{index} of {order}')

        # Remove the vertex at index
        graph.delete_vertex(index)

        # If the new chromatic number is not smaller than the current one. Is critical is false
        if vertex_coloring(graph, k=k-1, value_only=True) == False:

            # Is critical is false
            is_critical = False

            break
        
    #Reset the graph to replace deleted vertex
    graph = Graph(original_graph)
    
    # Return is critical flag
    return is_critical


def save(string, order, chromatic_number):

    # Store this graph in the grpahs folder
    f = open(f'{path}/order{order}_chi{chromatic_number}.txt', "a")
        
    # Write to file
    f.write(f'{string}\n')

    # CLose the filesave_c
    f.close()

def save_c(string, order):

    # Store this graph in the grpahs folder
    f = open(f'{critical_path}/circ{order}_crit{k}.txt', "a")

    # Write to file
    f.write(f'{string}\n')

    # CLose the file
    f.close()


# Attain the graph string from circulant graph
def circulant(n, L):

    E = []
    
    for i in range(n):

        for j in range(i+1, n):

            if(((i-j) % n) in  L):

                if({i,j} not in E):

                    E.append({i,j})
                    
    graph = Graph(E)
    
    graph_string = graph.graph6_string()

    return graph, graph_string


# Loop through all graphs with {min_vertices} to {max_vertices}
for vertice in range(min_vertices, max_vertices + 1):

    # For every grpah in the list
    for circulant_number in range(2, max_vertices):

        # Defien the path of the graph
        graph_path = f'{raw_path}/circ{vertice}.{circulant_number}.txt'

        try:
            # Open up and read the file graph
            f = open(graph_path, "r")

            # FOr every line in the file
            for line in f:

                results = line.split(None)

                results = list(map(int, results))

                # Create a list of nauty_geng connected graphs
                raw_graphs.append([vertice, results])  # add only first word

            # CLose the file
            f.close()

        except:

            pass

# For all the raw graphs
for (index, graph_data) in enumerate(raw_graphs):
    
    print(f'{index} out of {len(raw_graphs)} ~ {graph_data}')

    # Get the graphs chromatic number and save it to a new file
    graph, graph_string = circulant(*graph_data)
    
    order = graph.order()

    if critical_check(graph_string) == True:
        
        # Save this graph
        save_c(graph_string, order)
        

    # Save this graph
    # print('. . . Getting graph chromatic number')
    # chromatic_number = graph.chromatic_number()
    # save(graph_string, order, chromatic_number)
