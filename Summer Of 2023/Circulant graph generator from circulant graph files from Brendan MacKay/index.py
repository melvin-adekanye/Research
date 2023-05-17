import os
import time
import shutil
from sage.graphs.graph_coloring import vertex_coloring
from sage import *

# The selected chromatic number
k = int(input('k='))

# The raw and regular graphs
raw_graphs = []
graphs = []

min_vertices = 5  # circ-5 graphs
max_vertices = 50  # circ-50 graphs

print(f'**** STARTING AT {min_vertices} Vertices ****')

# Ask to delete old graph - X data
x = input(f'delete "/graphs - {k}" (y/n): ')

# Create the path for the graphs to be stored
graph_path = f'{os.getcwd()}/graphs - {k}'
source_path = f'{os.getcwd()}/raw graphs'

default_graph6_string_path = f'{graph_path}/all graphs'
default_params_path = f'{graph_path}/all graphs params'

critical_graph6_string_path = f'{graph_path}/critical graphs'
critical_params_path = f'{graph_path}/critical graphs params'

# If yes
if x == 'y':

    # Remove graphs
    shutil.rmtree(graph_path, ignore_errors=True)

    # Wait a bit
    time.sleep(3)

    # Then create a new folder
    os.mkdir(graph_path)

    # Create the sub paths
    os.mkdir(default_graph6_string_path)
    os.mkdir(default_params_path)
    os.mkdir(critical_graph6_string_path)
    os.mkdir(critical_params_path)

# Critical check. Takes in the graph6_string, graph raw string and the chromatic number


def critical_check(graph, chromatic_number):

    # Set the original graph before messing with it
    original_graph = Graph(graph)

    # if there is no cn
    if chromatic_number == None:

        # Get it from the graph
        chromatic_number = original_graph.chromatic_number()

    # Only if the CN is K
    if chromatic_number == k:

        # Get the order of the graph
        order = original_graph.order()

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

        # Return is_critical
        return is_critical

    # Return not critical to k value
    return False


# Save the graph in general


def save(type, string, raw_string, order, chromatic_number):

    if type == 'default':

        # Store this graph in the grpahs folder
        f = open(
            f'{default_graph6_string_path}/circ{order}_chi{chromatic_number}.txt', "a")
        e = open(
            f'{default_params_path}/circ{order}_chi{chromatic_number}_params.txt', "a")

        # Write to file
        f.write(f'{string}\n')
        e.write(f'{raw_string}\n')

        # CLose the file save
        f.close()
        e.close()

    elif type == 'critical':

        # Store this graph in the grpahs folder
        f = open(f'{critical_graph6_string_path}/circ{order}_crit{k}.txt', "a")
        e = open(f'{critical_params_path}/circ{order}_crit{k}_params.txt', "a")

        # Write to file
        f.write(f'{string}\n')
        e.write(f'{raw_string}\n')

        # CLose the file
        f.close()
        e.close()


# Convert array to string
def convert_array_to_string(array):

    total_string = ''

    # traverse in the string
    for (i, string) in enumerate(array):

        total_string += str(string)

        # Don't add space after last character
        if i < len(array)-1:

            total_string += ' '

    # Return the total string
    return total_string

# Attain the graph string from circulant graph


def circulant(n, L):

    E = []

    for i in range(n):

        for j in range(i+1, n):

            if(((i-j) % n) in L):

                if({i, j} not in E):

                    E.append({i, j})

    # Return the graph
    return E


print('. . . Gathering graph data files from raw graphs folder')
# Loop through all graphs with {min_vertices} to {max_vertices}
for order in range(min_vertices, max_vertices + 1):

    # For every grpah in the list (2 is the minimum chromatic number in raw graphs)
    for circulant_number in range(max_vertices):

        # Defien a  path to the graph (may not actually exist)
        path = f'{source_path}/circ{order}.{circulant_number}.txt'

        # Try: If the graph_path does exist
        try:

            # Open up and read the file graph
            f = open(path, "r")

            # FOr every line in the file
            for line in f:

                # Attain the raw graph numbers
                string = line.split(None)

                string = list(map(int, string))

                # Append a list to the raw graph
                raw_graphs.append([order, string])

            # CLose the file
            f.close()

        except:

            # If the graph path doesn't exist skip
            pass

print('. . . Analyzing graph data')
# For all the raw graphs
for (index, data) in enumerate(raw_graphs):

    # Define the order
    order = data[0]

    # Define the graph raw string (5 7 10 12)
    graph_raw_string = data[1]

    # Remove array props from the graph_raw_string
    graph6_raw_array_to_string = convert_array_to_string(graph_raw_string)

    # graph = circulant(first_elem, remaining_elems)
    E = circulant(order, graph_raw_string)

    # Create graphs and get details
    graph = Graph(E)
    graph6_string = graph.graph6_string()
    chromatic_number = graph.chromatic_number()

    # Check if this graph is critical
    is_critical = critical_check(graph, chromatic_number)

    # If this graph is critical
    if is_critical == True:

        # Save to critical
        save('critical', graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)

    # Save the graph
    save('default', graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)

    # Log out
    print(f'{index} out of {len(raw_graphs)} ~ {graph6_raw_array_to_string}')


print(f'k={k}')
print(f'Completed - Created ./graph - {k}')
