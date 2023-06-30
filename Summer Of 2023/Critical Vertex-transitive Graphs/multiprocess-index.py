import os
import time
import shutil
from sage.graphs.graph_coloring import vertex_coloring
from sage import *
import copy
import multiprocessing
import datetime

# The selected chromatic number
k = int(input('k='))

# The raw and regular graphs
raw_graphs = []
graphs = []

min_vertices = 5  # circ-5 graphs
max_vertices = 50  # circ-50 graphs

num_threads = 3

checkedGraphs = multiprocessing.Queue()


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

def threadChecks(graphDataArr):
   
    for graphData in graphDataArr:

        # Define the order
        order = graphData[0]

        # Define the graph raw string (5 7 10 12)
        graph_raw_string = graphData[1]

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
            checkedGraphs.put(['critical', graph6_string,
                 graph6_raw_array_to_string, order, chromatic_number])

        # Save the graph
        checkedGraphs.put(['default', graph6_string,
                 graph6_raw_array_to_string, order, chromatic_number])
       

        # Log out
        print(f'{index} out of {len(raw_graphs)} ~ {graph6_raw_array_to_string}')
       

       
print("Starting!")

start_time = datetime.datetime.now()

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

       
#Make a 2D array for thread tasks (will become 3D later in code)
#1st dimension stores the thread to use
#2nd dimension stores the graph arguments that each thread is in charge of
#3rd dimension stores the arguments for each graph
threadTasks = []
for i in range(num_threads):
    threadTasks.append([])

#Variables to use for multithreading
taskAssign = 0
allGraphs = 0


#Create the pool and start multithreading the graph calculations
pool = multiprocessing.Pool()

print('. . . Analyzing graph data')
# For all the raw graphs
for (index, data) in enumerate(raw_graphs):

   
    threadTasks[taskAssign].append([index, data])
   
    taskAssign = taskAssign + 1
    allGraphs = allGraphs + 1

    if (taskAssign == num_threads):
        taskAssign = 0

       
# This actually does the real multithreading
pool.map(threadChecks, threadTasks)

#This waits for the threads to finish
pool.close()
pool.join()


 
#Saving the graphs
outputLength = 0
outputTarget = checkedGraphs.qsize()

while outputLength != outputTarget:
   
    saveStuff = checkedGraphs.get()
   
    graphType = saveStuff[0]
    graph6_string = saveStuff[1]
    graph6_raw_array_to_string = saveStuff[2]
    order = saveStuff[3]
    chromatic_number = saveStuff[4]
   
    # If this graph is critical
    if graphType == 'critical':
       
        # Save to critical
        save('critical', graph6_string,
             graph6_raw_array_to_string, order, chromatic_number)

    # The critical graphs get saved in two spots
    save('default', graph6_string,
         graph6_raw_array_to_string, order, chromatic_number)

    # Log out
    print(f'{index} out of {len(raw_graphs)} ~ {graph6_raw_array_to_string}')
   
    outputLength = outputLength + 1


print(f'k={k}')
print(f'Completed - Created ./graph - {k}')


end_time = datetime.datetime.now()

elapsed_time = end_time - start_time
totalSeconds = elapsed_time.total_seconds()



hours, remainder = divmod(int(totalSeconds), 3600)
minutes, seconds = divmod(remainder, 60)
roundSeconds = float("{:.2f}".format(math.modf(totalSeconds)[0] + seconds))

print("Elapsed time: {} hours, {} minutes, {} seconds".format(int(hours), int(minutes), roundSeconds))