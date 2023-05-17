from sage.all import *
from operator import itemgetter

# Loop through all graphs with 3 - 7 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 7

# Create groupings
groups = []

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

        # Get its chromatic number
        chromatic_number = graph.chromatic_number()

        # Create a set [ the graph, the chromatic number ]
        set = [graph, chromatic_number]

        # Append the set to the groups
        groups.append(set)

        # Save the graphs in graph6 format
        graph6_string = graph.graph6_string()
        
        # Store this graph in the grpahs folder
        f = open(f'./graphs/order{vertice}_chi{chromatic_number}.txt', "a")
        
        # Write to file
        f.write(f'{graph6_string}\n')
        
        # CLose the file
        f.close()


# Sort the groups by the chromatic_number (1 = index of the chromatic_number)
groups = sorted(groups, key=itemgetter(1))
keep_going = True

# Display graphs
while keep_going:

    title = """

    / ___|  ___  _ __| |_(_)_ __   __ _  | __ ) _   _ 
    \___ \ / _ \| '__| __| | '_ \ / _` | |  _ \| | | |
    ___) | (_) | |  | |_| | | | | (_| | | |_) | |_| |
    |____/ \___/|_|   \__|_|_| |_|\__, | |____/ \\__, |
                    |___/         |___/ 
    ____ _                               _   _      
    / ___| |__  _ __ ___  _ __ ___   __ _| |_(_) ___ 
    | |   | '_ \| '__/ _ \| '_ ` _ \ / _` | __| |/ __|
    | |___| | | | | | (_) | | | | | | (_| | |_| | (__ 
    \____|_| |_|_|  \___/|_| |_| |_|\__,_|\__|_|\___|
                                                    
    _   _                 _                   
    | \ | |_   _ _ __ ___ | |__   ___ _ __ ___ 
    |  \| | | | | '_ ` _ \| '_ \ / _ \ '__/ __|
    | |\  | |_| | | | | | | |_) |  __/ |  \__ \\
    |_| \_|\__,_|_| |_| |_|_.__/ \___|_|  |___/

    
    """

    print(title)
    # print('\n\nSorting By Chromatic Numbers')
    print('\nPress \'a\' to show all groups (array)')
    print('\nPress \'q\' to quit')
    print('\n...\n')

    # Get the selected chromatic number
    selected_chromatic_number = input(
        f'Display Graphs with Chromatic Number: (2 - {max_vertices}) ')

    # Show the groups on request
    if (selected_chromatic_number == 'a'):

        print(groups)
        keep_going = False
        break

    # Escape
    if (selected_chromatic_number == 'q'):

        keep_going = False
        break

    # Print out the selected graphs
    selected_graphs = [x[0]
                       for x in groups if x[1] == int(selected_chromatic_number)]
    print(
        f'\n\n...\n{len(selected_graphs)} match the selected Chromatic Number')
    print(f'\n\n...\nSelected Graphs')

    # Iterate through all the sets in the group
    for set in groups:

        # Unpack the set
        graph = set[0]
        chromatic_number = set[1]

        # If the chromatic number is equal to the selected chromatic number
        if chromatic_number == int(selected_chromatic_number):

            # Show the graph
            show(graph)
