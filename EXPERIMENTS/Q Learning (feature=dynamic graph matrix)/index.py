import os
import torch
import numpy
import time
import math
from random import choice
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optimizer
from sage.all import *

# max Graph string length
MAX_GRAPH_VERTICE_LENGTH = 15**2
NUMBER_OF_CLASSES = 2  # Two classes ( critical or not critical )
LEARNING_RATE = 0.0003  # 0.003 # Decent learning rate

CLASSIFICATIONS = [0, 1]  # 0 = not critical 1 = critical

# The agent will have [X] number of data points to base future decisions on
MAX_MEMORY_SIZE = 500000
BATCH_SIZE = 500  # Batch size of 50 data points before learning

# Used in training to break when the agent is basically the Lebron James of classifying graphs
# An average classification score of [X] will break the training
BREAK_WHEN_AVERAGE_SCORE = 300  # 300
NUMBER_OF_ROUNDS = 10000  # Number of training rounds 5,000 is a great start.

# Agent variables
GAMMA = 0.99
EPSILON = 1
MIN_EPSILON = 0.01
DECAY_EPSILON = 1e-4

# The environment (which is the graph)


class ENV(nn.Module):

    def __init__(self):

        # Will cOntain all grpahs
        self.graphs = []

        # Will cOntain all test grpahs
        self.tests = []

        # Number of critical graphs
        self.num_critical = 0

        # Number of not critical graphs
        self.num_not_critical = 0

        # Create the path for the graphs to be stored
        path = f'{os.getcwd()}/graphs'

        # Defien the path of the graph
        not_critical_graphs_path = f'{path}/not_critical.txt'

        # Defien the path of the graph
        critical_graphs_path = f'{path}/critical.txt'

        # Open up and read the file graph
        f = open(not_critical_graphs_path, "r")

        # Open up and read the file graph
        cf = open(critical_graphs_path, "r")

        # FOr every line in the file
        for index, graph_string in enumerate(f):

            # Rean in the graph string without \n
            graph_string = graph_string.rstrip('\n')

            # Create the graph
            graph = Graph(graph_string)

            # Label is 0 (not critical)
            label = 0

            # Incriment the num_not_critical
            self.num_not_critical += 1

            # APpend this data to the not_critical_graphs
            self.graphs.append([graph_string, label])

        # FOr every line in the file
        for index, graph_string in enumerate(cf):

            # Rean in the graph string without \n
            graph_string = graph_string.rstrip('\n')

            # Create the graph
            graph = Graph(graph_string)

            # Label is 1 (critical)
            label = 1

            # Incriment the num_critical
            self.num_critical += 1

            # APpend this data to the not_critical_graphs
            self.graphs.append([graph_string, label])

        # Add random graphs from self.graphs to self.tests
        critical_graohs = [graph for graph in self.graphs if graph[1] == 1]
        not_critical_graohs = [graph for graph in self.graphs if graph[1] == 0]

        if len(critical_graohs) > 1:
            self.tests.append(choice(critical_graohs))
            self.tests.append(choice(critical_graohs))
            self.tests.append(choice(critical_graohs))
            self.tests.append(choice(critical_graohs))
            self.tests.append(choice(critical_graohs))

        if len(not_critical_graohs) > 1:
            self.tests.append(choice(not_critical_graohs))
            self.tests.append(choice(not_critical_graohs))
            self.tests.append(choice(not_critical_graohs))
            self.tests.append(choice(not_critical_graohs))
            self.tests.append(choice(not_critical_graohs))

        # Remove test graphs from the training set (self.graphs)
        self.graphs = [
            graph for graph in self.graphs if graph not in self.tests]

        # Print out
        print(f'Tests: ({len(self.tests)}) ==>> {self.tests}\n\n')
        print(f'{self.num_critical} critical graphs')
        print(f'{self.num_not_critical} not critical graphs\n\n')

        # Wait a bit to read print out

    # Return the graph matrix and graph string

    def graph(self, graph_string):

        graph_string = graph_string or self.random_graph()

        # Create the graph
        graph = Graph(graph_string)

        # Create the matrix
        matrix = graph.adjacency_matrix()

        # COnvetr the matrix from array to a torch tensor
        matrix = torch.tensor(
            numpy.array([matrix]),
            dtype=torch.float
        )

        # COnvert the matrix tensor to a single tensor
        matrix = matrix.reshape(-1)

        # Add zeros if graph string but numbers is less than MAX_GRAPH_VERTICE_LENGTH
        missing_length = MAX_GRAPH_VERTICE_LENGTH - len(matrix)

        # Spacer is an array of -1
        spacer = [-1 for i in range(missing_length)]

        # Convert spacer to a tensor
        spacer = torch.tensor(spacer)

        # Add spacer to matrix
        matrix_n_spacer = torch.cat((matrix, spacer))

        # Return the matrix
        return matrix_n_spacer, graph_string

    # Return a random graph from self.graphs

    def random_graph(self):

        # Pick graph[0] (the graph string)
        graph_string = choice(
            [graph[0]
                for graph in self.graphs]
        )

        return graph_string

    def get_label(self, requested_graph_string):

        # Pick graph[1] (the graph label. 0 or 1 for crit or not crit...)
        label = [graph[1]
                 for graph in self.graphs if graph[0] == requested_graph_string]

        # If the requested_graph_string is not found in the self.graphs array
        if len(label) == 0:

            # Check if this graph is in the tests
            label = [graph[1]
                     for graph in self.tests if graph[0] == requested_graph_string]

            # If it's still not found
            if len(label) == 0:

                # This must be a new graph
                label = 'unknown'
            else:

                label = label[0]

        else:

            # Else if it is found. Remove the array to get the int value (0 or 1)
            label = label[0]

        # Return the label
        return label

    def step(self, graph_string, classification):

        label = self.get_label(graph_string)

        # If there is no label
        if label == -1:

            # Reward is not changed
            reward = 0

            # Done is true
            done = True

        else:

            # If the agent had the wrong classification
            if classification != label:

                # Reqard is bad. End test
                reward = -10

                # Done is true
                done = True

            else:

                # Reqard is good. Kee going
                reward = 1

                # Done is false
                done = False

        info = f'. . . Agent\'s Classification: {classification} | {graph_string} | Correct Classification: {label}'
        # print(info)

        # Get a new and random graph
        graph, graph_string = self.graph(None)

        # Return / update the env
        return graph, graph_string, reward, done, info

# The Deep Q Network


class DQN(nn.Module):

    def __init__(self):

        super(DQN, self).__init__()

        self.fc1 = nn.Linear(MAX_GRAPH_VERTICE_LENGTH, 64)
        self.fc2 = nn.Linear(64, 128)
        self.fc3 = nn.Linear(128, 214)
        self.fc4 = nn.Linear(214, 214)
        self.fc5 = nn.Linear(214, 128)
        self.fc6 = nn.Linear(128, 64)
        self.out = nn.Linear(64, NUMBER_OF_CLASSES)

        self.optimizer = optimizer.Adam(self.parameters(), lr=LEARNING_RATE)
        self.loss = nn.MSELoss()

    def forward(self, state):

        state = F.relu(
            self.fc1(state)
        )

        state = F.relu(
            self.fc2(state)
        )

        state = F.relu(
            self.fc3(state)
        )

        state = F.relu(
            self.fc4(state)
        )

        state = F.relu(
            self.fc5(state)
        )

        state = F.relu(
            self.fc6(state)
        )

        # get the classifications
        classifications = self.out(state)

        return classifications


# The Agent

class AGENT():

    def __init__(self):

        self.gamma = GAMMA
        self.epsilon = EPSILON

        self.classifications = CLASSIFICATIONS
        self.memory_size = MAX_MEMORY_SIZE
        self.batch_size = BATCH_SIZE

        self.memory_cursor = 0

        self.evaluation = DQN()

        self.state_memory = numpy.zeros(
            (self.memory_size, MAX_GRAPH_VERTICE_LENGTH), dtype=numpy.float32)
        self.new_state_memory = numpy.zeros(
            (self.memory_size, MAX_GRAPH_VERTICE_LENGTH), dtype=numpy.float32)
        self.action_memory = numpy.zeros(
            self.memory_size, dtype=numpy.int32)
        self.reward_memory = numpy.zeros(
            self.memory_size, dtype=numpy.float32)

    def store_transition(self, state, action, reward, new_state):

        index = self.memory_cursor % self.memory_size

        self.state_memory[index] = state
        self.action_memory[index] = action
        self.reward_memory[index] = reward
        self.new_state_memory[index] = new_state

        self.memory_cursor += 1

    def classify(self, graph):

        random_number = numpy.random.random()

        if random_number > self.epsilon:

            classifications = self.evaluation(graph)

            classification = torch.argmax(classifications).item()

        else:

            classification = numpy.random.choice(CLASSIFICATIONS)

        return classification

    def learn(self):

        if self.memory_cursor < self.batch_size:

            return

        self.evaluation.optimizer.zero_grad()

        max_memory = min(self.memory_cursor, self.memory_size)

        batch = numpy.random.choice(max_memory, self.batch_size, replace=False)

        batch_index = numpy.arange(self.batch_size, dtype=numpy.int32)

        state_batch = torch.tensor(
            self.state_memory[batch]
        )

        new_state_batch = torch.tensor(
            self.new_state_memory[batch]
        )

        reward_batch = torch.tensor(
            self.reward_memory[batch]
        )

        action_batch = self.action_memory[batch]

        evaluation = self.evaluation(state_batch)[batch_index, action_batch]

        next = self.evaluation(new_state_batch)

        target = reward_batch + self.gamma * torch.max(next, dim=1)[0]

        loss = self.evaluation.loss(target, evaluation)

        loss.backward()

        self.evaluation.optimizer.step()

        self.epsilon = self.epsilon - DECAY_EPSILON\
            if self.epsilon > MIN_EPSILON else MIN_EPSILON


if __name__ == "__main__":

    env = ENV()
    agent = AGENT()

    scores, epiilon_history = [], []
    average_score = 0

    # Set start time
    start_time = time.time()

    for round_ in range(NUMBER_OF_ROUNDS):

        score = 0

        done = False

        graph, graph_string = env.graph(None)

        if average_score > BREAK_WHEN_AVERAGE_SCORE:

            break

        # and average_score < 200. Forces a stop in training after achieving such success!
        while not done:

            classification = agent.classify(graph)

            new_graph, new_graph_string, reward, done, info = env.step(
                graph_string, classification)

            score += reward

            agent.store_transition(graph, classification, reward, new_graph)

            agent.learn()

            graph, graph_string = new_graph, new_graph_string

        scores.append(score)

        epiilon_history.append(agent.epsilon)

        average_score = numpy.mean(
            scores[-50:]
        )

        # Print out summary
        print(f'Round {round_} / {NUMBER_OF_ROUNDS} - Epsilon {round(agent.epsilon, 2)} - Graph {graph_string} - Avg Score {average_score}')

    # Set end time (not a Prophet tho)
    end_time = time.time()

    # Calculate the time taken
    time_taken = end_time - start_time

    print(f'\n\nTime Taken {round(time_taken, 2)}s')

    print(f'Variables')
    print(f'MAX_GRAPH_VERTICE_LENGTH => {MAX_GRAPH_VERTICE_LENGTH}')
    print(f'NUMBER_OF_CLASSES => {NUMBER_OF_CLASSES}')
    print(f'LEARNING_RATE => {LEARNING_RATE}')
    print(f'CLASSIFICATIONS => {CLASSIFICATIONS}')
    print(f'MAX_MEMORY_SIZE => {MAX_MEMORY_SIZE}')
    print(f'BATCH_SIZE => {BATCH_SIZE}')
    print(f'BREAK_WHEN_AVERAGE_SCORE => {BREAK_WHEN_AVERAGE_SCORE}')
    print(f'NUMBER_OF_ROUNDS => {NUMBER_OF_ROUNDS}')

    print(f'GAMMA => {GAMMA}')
    print(f'EPSILON => {EPSILON}')
    print(f'MIN_EPSILON => {MIN_EPSILON}')
    print(f'DECAY_EPSILON => {DECAY_EPSILON}')

    print(f'\n\n. . . Tests . . . \n\n')

    for test in env.tests:

        graph_string = test[0]

        graph, _ = env.graph(graph_string)

        start_classify_time = time.time()

        classification = agent.classify(graph)

        stop_classify_time = time.time()

        _, _, _, _, info = env.step(graph_string, classification)

        print(f'Order #{Graph(graph_string).order()}')
        print(info)

        classify_time_taken = round(
            stop_classify_time - start_classify_time, 7)

        print(f'. . . . . . Time Taken To Classify: {classify_time_taken}s')

    while True:

        try:

            # Query the user for a graph stirng
            requested_graph_string = str(
                input('\n\nEnter a graph string to check criticality? (enter q to quit) '))

            if requested_graph_string == 'q':
            
                break

            # Create the graph object
            graph_string = requested_graph_string

            graph, _ = env.graph(graph_string)
            classification = agent.classify(graph)
            _, _, _, _, info = env.step(graph_string, classification)

            print('\n', info)

        except:

            print('( X ) Invalid Graph String')

    # Order #8 Critical Graph -> GQjuv{
    # Order #8 Not Critical Graph -> GCY^Bw
