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

# The max graph order (3 - 10 vertices)
MAX_GRAPH_ORDER = 5**2
NUMBER_OF_CLASSES = 2
LEARNING_RATE = 0.003
NUMBER_OF_ROUNDS = 5000

CLASSIFICATIONS = [0, 1]

MAX_MEMORY_SIZE = 100000
BATCH_SIZE = 50

GAMMA = 0.99
EPSILON = 1
MIN_EPSILON = 0.01
DECAY_EPSILON = 1e-4

# The environment (which is the graph)


class ENV(nn.Module):

    def __init__(self):

        self.graphs = []

        # Create the path for the graphs to be stored
        path = f'{os.getcwd()}/graphs'

        # Defien the path of the graph
        not_critical_graphs_path = f'{path}/not_critical_graphs.txt'

        # Defien the path of the graph
        critical_graphs_path = f'{path}/critical_graphs.txt'

        # Open up and read the file graph
        f = open(not_critical_graphs_path, "r")

        # Open up and read the file graph
        cf = open(critical_graphs_path, "r")

        # FOr every line in the file
        for index, graph_string in enumerate(f):

            graph_string = graph_string.rstrip('\n')
            
            graph = Graph(graph_string)

            if graph.order() == math.sqrt(MAX_GRAPH_ORDER):

                # Label is 0 (not critical)
                label = 0

                # APpend this data to the not_critical_graphs
                self.graphs.append([graph_string, label])

        # FOr every line in the file
        for index, graph_string in enumerate(cf):

            graph_string = graph_string.rstrip('\n')

            graph = Graph(graph_string)

            if graph.order() == math.sqrt(MAX_GRAPH_ORDER):

                # Label is 1 (critical)
                label = 1

                # APpend this data to the not_critical_graphs
                self.graphs.append([line, label])        
        
        # FOr some reasons functions are slipped into the  graphs array
        self.graphs = [graph for graph in self.graphs if callable(graph[0]) == False]
    
    
    # Return the state

    def graph(self, graph_string):

        graph_string = graph_string or self.random_graph()

        # Create the graph
        graph = Graph(graph_string)

        # Create the matrix
        matrix = graph.adjacency_matrix()

        matrix = torch.tensor(
            numpy.array([matrix]),
            dtype=torch.float
        )

        # COnvert the matrix to a single list
        matrix = matrix.reshape(-1)

        # Return the matrix
        return matrix, graph_string

    def random_graph(self):

        graph_string = choice(
            [graph[0]
                for graph in self.graphs ]
        )
        
        return graph_string

    def get_label(self, requested_graph_string):

        label = [graph[1]
                 for graph in self.graphs if graph[0] == requested_graph_string]

        # label is [0] or [1]. Remove from array
        label = label[0]

        return label

    def step(self, graph_string, classification):

        label = self.get_label(graph_string)

        if classification != label:

            reward = -10
            done = True

        else:

            reward = 1
            done = False
        
        info = f'classification: {classification} | label: {label}'

        graph, graph_string = self.graph(None)

        return graph, graph_string, reward, done, info

# The Deep Q Network


class DQN(nn.Module):

    def __init__(self):

        super(DQN, self).__init__()

        self.fc1 = nn.Linear(MAX_GRAPH_ORDER, 64)
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
            (self.memory_size, MAX_GRAPH_ORDER), dtype=numpy.float32)
        self.new_state_memory = numpy.zeros(
            (self.memory_size, MAX_GRAPH_ORDER), dtype=numpy.float32)
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

        if average_score > 50:
            
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
        print(f'Round {round_} / {NUMBER_OF_ROUNDS} - Epsilon {round(agent.epsilon, 2)} - Graph Order {math.sqrt(MAX_GRAPH_ORDER)} - Graph {graph_string} - Avg Score {average_score}')
    
    # Set end time (not a Prophet tho)
    end_time = time.time()

    # Calculate the time taken
    time_taken = end_time - start_time

    print(f'TIme Taken {round(time_taken, 2)}s')

    while True:

        # Query the user for a graph stirng
        requested_graph_string = str(
            input('\n\n. . . Test . . .\n\nEnter a graph string to check criticality? '))
        
        # Create the graph object
        graph_string = requested_graph_string

        graph, _ = env.graph(graph_string)
        classification = agent.classify(graph)
        _, _, _, _, info = env.step(graph_string, classification)

        print('\n\nEvaluation\n', info)


# DUw