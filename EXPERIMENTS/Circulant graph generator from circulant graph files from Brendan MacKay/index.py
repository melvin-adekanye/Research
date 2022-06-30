import os
import time
import shutil
from sage.graphs.graph_coloring import vertex_coloring

# The selected chromatic number
k = 5

# The raw and regular graphs
raw_graphs = []
graphs = []

min_vertices = 5  # circ-5 graphs
max_vertices = 30  # circ-50 graphs

# Ask to delete old graph - X data
x = input(f'delete "/graphs - {k}" (y/n): ')

# If yes
if x == 'y':

    # Create the path for the graphs to be stored
    graph_path = f'{os.getcwd()}/graphs - {k}'
    raw_path = f'{os.getcwd()}/raw graphs'

    path = f'{graph_path}/graph6_string'
    grapH_raw_path = f'{graph_path}/raw'

    critical_path = f'{graph_path}/critical graphs'
    critical_raw_path = f'{graph_path}/critical graphs raw'

    # Remove graphs
    shutil.rmtree(graph_path, ignore_errors=True)

    # Wait a bit
    time.sleep(3)

    # Then create a new folder
    os.mkdir(graph_path)

    # Create the sub paths
    os.mkdir(path)
    os.mkdir(raw_path)
    os.mkdir(critical_path)
    os.mkdir(critical_raw_path)

# Critical check. Takes in the graph6_string, graph order and the chromatic number


def critical_check(string, order, chromatic_number):

    # If the chromatic number is not k
    if chromatic_number != k:

        # Return not critical
        return False

    # Create the graph using the string
    graph = Graph(string)
    original_graph = Graph(string)

    # A flag to check if a graph is critical
    is_critical = True

    # Iterate through every vertice in graphs vertices
    for index in range(order):

        # print out the vertex being deleted
        print(f'. . . Deleting Vertice-{index} of {order}')

        # Remove the vertex at index
        graph.delete_vertex(index)

        # After deleting a vertex, can we color this graph with less colors. (Faster than re-caculating the chormatic number)
        if vertex_coloring(graph, k=k-1, value_only=True) == False:

            # Is critical is false
            is_critical = False

            # Not critical, lets break the loop
            break

    # Reset the graph to replace deleted vertex
    graph = Graph(original_graph)

    # Return is critical flag
    return is_critical

# Save the graph in general


def save(string, raw_string, order, chromatic_number):

    # Store this graph in the grpahs folder
    f = open(f'{path}/circ{order}_chi{chromatic_number}.txt', "a")
    e = open(f'{grapH_raw_path}/circ{order}_chi{chromatic_number}.txt', "a")

    # Write to file
    f.write(f'{string}\n')
    e.write(f'{raw_string}\n')

    # CLose the file save
    f.close()
    e.close()


def save_critical(string, raw_string, order):

    # Store this graph in the grpahs folder
    f = open(f'{critical_path}/circ{order}_crit{k}.txt', "a")
    e = open(f'{critical_raw_path}/circ{order}_crit{k}.txt', "a")

    # Write to file
    f.write(f'{string}\n')
    e.write(f'{raw_string}\n')

    # CLose the file
    f.close()
    e.close()

# Attain the graph string from circulant graph


def circulant(n, L):

    E = []

    for i in range(n):

        for j in range(i+1, n):

            if(((i-j) % n) in L):

                if({i, j} not in E):

                    E.append({i, j})

    graph = Graph(E)

    graph_string = graph.graph6_string()

    return graph, graph_string


# Loop through all graphs with {min_vertices} to {max_vertices}
for vertice in range(min_vertices, max_vertices + 1):

    # For every grpah in the list (2 is the minimum chromatic number in raw graphs)
    for circulant_number in range(0, max_vertices):

        # Defien the path of the graph
        graph_path = f'{grapH_raw_path}/circ{vertice}.{circulant_number}.txt'

        try:
            # Open up and read the file graph
            f = open(graph_path, "r")

            # FOr every line in the file
            for line in f:

                # Attain the raw graph numbers
                results = line.split(None)
                results = list(map(int, results))

                # Create a list of nauty_geng connected graphs
                raw_graphs.append([vertice, results, circulant_number])  # add only first word

            # CLose the file
            f.close()

        except:

            pass

# For all the raw graphs
for (index, graph_data) in enumerate(raw_graphs):

    # Define the raw_graph_string
    raw_graph_string = f"{graph_data[0]} {' '.join([str(x) for x in graph_data[1]])}"

    print(f'{index} out of {len(raw_graphs)} ~ {graph_data}')

    # Get the graphs chromatic number and save it to a new file
    graph, graph_string = circulant(*graph_data)

    # get the order of the graph
    order = graph.order()

    # Check if this graph is critical
    if critical_check(graph_string, order, chromatic_number) == True:

        # Save this graph
        save_critical(graph_string, raw_graph_string, order)

    # Save the graph
    save(graph_string, raw_graph_string, order, chromatic_number)
