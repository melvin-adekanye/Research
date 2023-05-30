import networkx as nx
import matplotlib.pyplot as plt
import os
import datetime
import os
import math
from concurrent.futures import ThreadPoolExecutor


#This is where we're saving the images to
imgDirectory = "images"

inputFile = ""

def drawFigureGraph(G, fig):

                        
    color_array = ["red", "blue", "green", "gold", "orange", "purple", "teal", "black", "grey", "steelblue", "magenta", "violet", "dodgerblue", "brown"]
    color_array_trunc = []
    
    for i in range(len(G.nodes())):
        color_array_trunc.append(color_array[i])
        
    # Create a dictionary that maps each node to its color
    # color_map = {node: color_array[coloring[node]] for node in adjacency_dict.keys()}

    # Draw the colored graph using networkx
    nx.draw(G, with_labels=True, node_color=color_array_trunc, font_color='w', ax=fig.add_subplot(111))


def adjacencyDict(edges):
        # Build a dictionary where each edge is a key and its value is a set of neighboring edges
    adjacency_dict = {}
    for edge in edges:
        if edge[0] not in adjacency_dict:
            adjacency_dict[edge[0]] = set()
        if edge[1] not in adjacency_dict:
            adjacency_dict[edge[1]] = set()
        adjacency_dict[edge[0]].add(edge[1])
        adjacency_dict[edge[1]].add(edge[0])
    
    return adjacency_dict


def showGraphsFromFile():
    inputStream = open("graph.g6", "r")
    for line in inputStream:
        
        graph = nx.from_graph6_bytes(bytes(line.strip(), encoding = "ascii"))
        showGraph(graph)


def multithreading(num_threads):

    global imgDirectory
    global inputFile
    
    if (not os.path.exists(imgDirectory)):
        os.makedirs(imgDirectory)
    
    # Create a thread pool executor with the specified number of threads

    with ThreadPoolExecutor(max_workers=num_threads) as executor:

        # Submit each number to the executor for processing

        futures = []
        
        inputStream = None
        
        try:
            inputStream = open("graph.g6", "r")
        except:
            print("No such file exists!")
            return
        for line in inputStream:

            graph = nx.from_graph6_bytes(bytes(line.strip(), encoding = "ascii"))
            futures.append(executor.submit(foo,networkx_graph))


        # Get the results from each future as they become available

        results = [future.result() for future in futures]
        
        # for r in results:
        #    foo2(r)
            
    return


def foo(graph):

    global writeToFile
    if (writeToFile):
        #Save the graph image
        path = os.path.join(imgDirectory, f"graph_{currentGraphFile}.png")
        figure = plt.figure()
        drawFigureGraph(graph, figure)
        plt.savefig(path)
        plt.close()

        currentGraphFile = currentGraphFile + 1

# Example usage

plt.ioff()

inputFile = input("What file do you want to import from? ")



print("Starting!")
start_time = datetime.datetime.now()

multithreading(3)

end_time = datetime.datetime.now()

elapsed_time = end_time - start_time
totalSeconds = elapsed_time.total_seconds()



hours, remainder = divmod(int(totalSeconds), 3600)
minutes, seconds = divmod(remainder, 60)
roundSeconds = float("{:.2f}".format(math.modf(totalSeconds)[0] + seconds))

print("Elapsed time: {} hours, {} minutes, {} seconds".format(int(hours), int(minutes), roundSeconds))