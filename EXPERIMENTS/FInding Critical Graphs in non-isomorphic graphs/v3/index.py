from sage.all import *
import os

import numpy

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim

# Create the path for the graphs to be stored
path = f'{os.getcwd()}/graphs'

# Number of inputs (features to determine if a graph is 0-not_critical or 1-critical)
number_of_features = 4

# Number of labels = 2 (critical or not critical)
number_of_labels = 2

# Defien the path of the graph
not_critical_graphs_path = f'{path}/not_critical_graphs.txt'

# Defien the path of the graph
critical_graphs_path = f'{path}/critical_graphs.txt'

# Randomly shuffle two arrays in the same way

def unison_shuffled_copies(a, b):

    assert len(a) == len(b)

    p = numpy.random.permutation(len(a))

    return a[p], b[p]


# Train the model


def train():

    print('. . . Training')

    # Create a number array
    not_critical_features = []

    # Create a number array
    not_critical_labels = []

    # Create a number array
    critical_features = []

    # Create a number array
    critical_labels = []

    # Open up and read the file graph
    f = open(not_critical_graphs_path, "r")

    # Open up and read the file graph
    cf = open(critical_graphs_path, "r")

    # FOr every line in the file
    for index, line in enumerate(f):

        graph_string = line.split(
            None, 1)[0]

        graph = Graph(graph_string)

        feature = [

            # Number of vertices
            graph.order(),

            # Number of edges
            # graph.size(),

            # Max degree
            max(graph.degree()),

            # Min degree
            min(graph.degree()),

            # Chromatic #
            graph.chromatic_number()

        ]

        # Label is 0 (not critical)
        label = 0

        # APpend this data to the not_critical_graphs
        not_critical_features.append(feature)

        # Append the labels
        not_critical_labels.append(label)

    # FOr every line in the file
    for index, line in enumerate(cf):

        graph_string = line.split(
            None, 1)[0]

        graph = Graph(graph_string)

        feature = [

            # Number of vertices
            graph.order(),

            # Number of edges
            # graph.size(),

            # Max degree
            max(graph.degree()),

            # Min degree
            min(graph.degree()),

            # Chromatic #
            graph.chromatic_number()

        ]

        # Label is 1 (critical)
        label = 1

        # APpend this data to the not_critical_graphs
        critical_features.append(feature)

        # Append the labels
        critical_labels.append(label)

    # CLose the file
    f.close()
    cf.close()

    # Define the features and labels
    all_features = not_critical_features + critical_features
    all_labels = not_critical_labels + critical_labels

    # COnvert the lists to numpy arrays
    all_features = numpy.array(all_features)
    all_labels = numpy.array(all_labels)

    # Randomly shuffle arrays
    # all_features, all_labels = unison_shuffled_copies(all_features, all_labels)

    # COnvert the not_critical_labels/features list to a numpy array
    all_features = torch.tensor(
        all_features, dtype=torch.float)
    all_labels = torch.tensor(
        all_labels, dtype=torch.long)

    # Test features & labels
    X, Y = (all_features, all_labels)

    for epoch in range(epoch_length):

        # Get the models output
        y = model(X)

        # Calculate the loss
        loss = F.nll_loss(y, Y)

        print(f'\nLoss: {loss}')
        print(f'Epoch {epoch} of {epoch_length}')

        # updating the Weights and biases
        model.zero_grad()

        # a little mix of back propagation
        loss.backward()

        # Step the optimizer
        optimizer.step()

# Test the model


def test():

    while True:

        # Query the user for a graph stirng
        requested_graph_string = str(input('Enter a graph string to check criticality? '))

        if requested_graph_string == 'q':
            
            break
        
        try:
            # Create the graph object
            graph = Graph(requested_graph_string)

            feature = torch.tensor([

                # Number of vertices
                graph.order(),

                # Number of edges
                # graph.size(),

                # Max degree
                max(graph.degree()),

                # Min degree
                min(graph.degree()),

                # Chromatic #
                graph.chromatic_number()

            ], dtype=torch.float)

            # Get the output
            y = model(feature)

            # Get the models prediction (tuple = (value, index))
            _, prediction = y.max(0)

            # Create a summary
            print(f'\n\nSummary\n\nnumber_of_features={number_of_features}\nnumber_of_labels={number_of_labels}\nlearning_rate={learning_rate}\nepoch_length={epoch_length}\nprediction is_critical: {bool(prediction)}')

        except:
            
            pass

# Neural network class


class Net(nn.Module):

    def __init__(self):

        print('. . . Initializing Neural Networks 💎')

        super(Net, self).__init__()

        self.fc1 = nn.Linear(number_of_features, 64)
        self.fc2 = nn.Linear(64, 128)
        self.fc3 = nn.Linear(128, 128)
        self.fc4 = nn.Linear(128, 214)
        self.fc5 = nn.Linear(214, 214)
        self.fc6 = nn.Linear(214, 128)
        self.fc7 = nn.Linear(128, 64)
        self.fc8 = nn.Linear(64, 64)
        self.out = nn.Linear(64, number_of_labels)

    def forward(self, X):

        X = F.relu(self.fc1(X))
        X = F.relu(self.fc2(X))
        X = F.relu(self.fc3(X))
        X = F.relu(self.fc4(X))
        X = F.relu(self.fc5(X))
        X = F.relu(self.fc6(X))
        X = F.relu(self.fc7(X))
        X = F.relu(self.fc8(X))
        X = self.out(X)

        # Note: the softmax operation will transform your input
        # into a probability distribution i.e. the sum of all elements will be 1
        return F.softmax(X, dim=0)


# Init the model
model = Net()

# Learning rate
learning_rate = 0.00005  # Best: 0.000005

# Number of training iterations
epoch_length = 1000

# Define the model Optimizer
optimizer = optim.Adam(model.parameters(), lr=learning_rate)

# Train
train()

# Test
test()





"""

# Path to get the data
path = f'../../../DATA/graphs'

# Loop through all graphs with 3 - 8 vertices (if runs quickly try 8) and separate them based on their chromatic number.
# Should get chromatic numbers between 2 and the number of vertices.

min_vertices = 3
max_vertices = 8

# Set start time
start_time = time.time()

graphs = []
critical_graphs = []
not_critical = []

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

    # Delete highest degree vertice first (False is faster)
    delete_vertice_with_highest_degree_first = False

    # Sort the indice_n_degree by the degree value
    indice_n_degree.sort(
        key=lambda x: x[1], reverse=delete_vertice_with_highest_degree_first)

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

print(f'TIme Taken {round(time_taken, 2)}s')
print('Done')

# print('critical_graphs: ', critical_graphs)
# print('not_critical: ', not_critical)
"""
