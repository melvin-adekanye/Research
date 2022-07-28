import os
import time
import shutil
from sage.graphs.graph_coloring import vertex_coloring
from sage import *

code = " 5 2 3 4 5 0 3 4 5 0 4 5 0 5 0 "

# Remove all spaces from the code
formatted_code = code.strip().replace(" ", "").split('0')

# Graph sequence
sequence = []

# Loop through the formatted code
for codes in formatted_code:

    if len(codes) == 1:

        break

    leading_vertex = codes[0]

    # Get the index in the codes
    for index in range(len(codes)):
            
        if index != 0:
        
            appending_vertex = codes[index]

            pair = [int(leading_vertex), int(appending_vertex)]

            sequence.append(pair)

print(sequence)

sequence.append([])

Graph(sequence).show()