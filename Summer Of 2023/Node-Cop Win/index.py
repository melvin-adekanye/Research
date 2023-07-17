from sage.graphs.graph import Graph
from math import factorial

# Combinations


def combinations(n, k):
    if k < 0 or k > n:
        return 0
    else:
        return factorial(n) // (factorial(k) * factorial(n - k))


# Function to check if vertex v is a corner in graph G
def is_corner(G, v):

    # Get all vertices of the graph
    V = G.vertices()

    # Get the neighbors of vertex v and create a set
    N = set(G.neighbors(v, closed=True))

    # Iterate over all vertices in the graph
    for u in V:

        # Exclude the vertex v itself
        if u != v:

            # Get the neighbors of vertex u and create a set
            M = set(G.neighbors(u, closed=True))

            # Check if the neighbors of v are a subset of the neighbors of u
            if N.issubset(M):

                # Vertex v is a corner
                return True

    # Vertex v is not a corner
    return False


# Takes a graph G and a probability (or variable) p and returns the propability that the graph
# is cop-win if each edge fails independently with probability p.
# We call this the ``cop-win reliability''
def CRel(G, p):
    P = 0
    E = G.edges()
    m = len(E)
    C = Combinations(E).list()
    for U in C:
        H = G.subgraph(edges=U)
        if is_copwin(H):
            k = len(H.edges())
            P += (p**(k))*((1-p)**(m-k))

    return P


# Function to calculate the non-cooperative reliability of a graph G given a probability p.
def NCRel(G, p):

    # Initialize the variable P to 0.
    P = 0

    # Get the vertices of graph G.
    V = G.vertices()

    # Calculate the number of vertices in graph G.
    n = len(V)

    # Generate all combinations of vertices using the Combinations() function and store them in C.
    C = Combinations(V).list()

    # Iterate through each combination of vertices U in C.
    for U in C:
        # Create a subgraph H by taking the vertices in U from G.
        H = G.subgraph(U)

        # Check if the subgraph H is cop-win.
        if is_copwin(H):
            # Calculate the order of the subgraph H.
            k = H.order()

            # Update the value of P by adding p raised to the power of (k * (1-p) raised to the power of (n-k)).
            P += (p**(k))*((1-p)**(n-k))

    # Return the calculated non-cooperative reliability P.
    return P


# Function to check if a graph G is cop-win.
def is_copwin(G):

    # Create a copy of the graph G and assign it to the variable H.
    H = Graph(G)

    # Get the vertices of graph H.
    V = H.vertices()

    # Calculate the number of vertices in graph H.
    n = len(V)

    # If there is only one vertex in the graph, it is cop-win.
    if n == 1:
        return True

    # Iterate through each vertex u in V.
    for u in V:
        # Check if the vertex u is a corner vertex.
        if is_corner(H, u):
            # Delete the vertex u from graph H.
            H.delete_vertex(u)

            # Recursively call the is_copwin() function on the modified graph H.
            return is_copwin(H)

    # If no corner vertex is found, the graph is not cop-win.
    return False


# Graphs with n vertices and (n choose 2)-(n-j) edges (graphs that are only missing j edges) for j=1, 0, -1, -2.
def cop_win(graphs):

    equations = []  # List to store the equations for each graph
    x = SR.var('x')  # Variable x for the symbolic ring
    m = -1  # Initialize the maximum value at 1/2
    noOptimal = False  # Flag to track if an optimal solution is found
    index = 0  # Keeps track of the position in the list of graphs

    maxGraph = None
    maxRel = None
    maxIndex = None
    N = None

    for graph in graphs:

        # Incriment
        index += 1
        # Create a graph object from the graph data
        G = Graph(graph)
        # Calculate the non-cooperative reliability polynomial for the graph
        N = CRel(G, x)
        # Evaluate the polynomial at x=1/2
        val = N.subs(x=1/2)

        # If a larger value is found at 1/2, update the variables
        if val > m:
            m = val
            maxGraph = G
            maxRel = N
            maxIndex = index

        # Write the polynomial to the equations list
        equations.append(str(N)+'\r\n')

    index = 0  # Reset the index counter

    #loopError = False
    # Loop to check for crossings in the interval (0, 1) for all NCRels except the maximum one
    for equation in equations:

        index += 1

        if index != maxIndex:

            # Convert the equation string back to a polynomial in the symbolic ring
            N = SR(equation)
            # print(RDF)

            # Calculate the difference between the polynomial and the maximum polynomial
            diff = N - maxRel
            # print(diff)

            # Find the real roots of the difference polynomial
            try:
                rts = N.roots(multiplicities=False, ring=RDF)
                # print(rts)

                rts = diff.roots(multiplicities=False, ring=RDF)

                for r in rts:

                    # Check if the root lies in the interval (0, 1)
                    if 0 < r < 0.99999999999:
                        noOptimal = True
                        break
            except:
                noOptimal = False

            if noOptimal:
                break

    if maxGraph == None:
        print('Inconclusive. No max grah or max reliability')
    elif noOptimal == False:

        print(maxRel)  # Display the maximum non-cooperative reliability polynomial
        maxGraph.show()  # Display the maximum graph

    else:
        print('More investigation required:')
        print(N)  # Display the polynomial with a crossing
        # Display the index of the polynomial with a crossing
        print('Root index', index)
        print(maxRel)  # Display the maximum non-cooperative reliability polynomial
        # Display the index of the maximum non-cooperative reliability polynomial
        print('Max Index', maxIndex)


# Function to create a regular graph with n vertices and degree k.
def generate_k_regular_graphs(n, k):

    # -d = min degree
    # Use the nauty_geng function to generate k-regular graphs
    graphs_iter = graphs.nauty_geng(f'{n} -c -d{k} -D{k}')

    # Create a list to store the generated k-regular graphs
    k_regular_graphs = [G for G in graphs_iter]

    # Return the list of generated k-regular graphs
    return k_regular_graphs


# Input n and k in terminal
n = int(input("Enter number of vertices (n):  "))
k = int(input("Enter number of degree (k):  "))

# Example usage
# k = n - 2  # Regularity
k_regular_graphs = generate_k_regular_graphs(n, k)

# Print the generated graphs
print(f"Analyzing Cop Win — K-Regular Graphs")
cop_win(k_regular_graphs)

# Graphs with n vertices and (n choose 2)-(n-j) edges (graphs that are only missing j edges) for j=1, 0, -1, -2.


def create_graph_n_C_2(n, j):
    m = combinations(n, 2) - (n - j)  # Number of edges

    if j > 1:
        raise ValueError("j must be -2, -1, 0, or 1.")

    graph = Graph(n)
    vertices = graph.vertices()

    for i in range(n-1):
        for j in range(i+1, n):
            if m > 0:
                graph.add_edge(vertices[i], vertices[j])
                m -= 1
            else:
                return graph

    return graph


j_values = [1, 0, -1, -2]
j_graphs = []

for j in j_values:
    graph = create_graph_n_C_2(n, j)

print(f"Analyzing Cop Win — nC2 Graphs")
cop_win(j_graphs)


# Graphs with n vertices and 3(n-2) edges for all n\ge 6 (I have some motivation that this class is interesting).
def create_graph_3n_2(n):

    num_vertices = 3 * n + 2
    # Adjust the probability as needed
    graph = graphs.RandomGNP(num_vertices, 0.5)

    return graph


# Example usage
graph = create_graph_3n_2(n)
print(f"Analyzing Cop Win — 3(n+2) Graphs")
cop_win([graph])
