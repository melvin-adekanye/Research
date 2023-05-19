from sage.graphs.graph_coloring import vertex_coloring


# Circulant generator
def circulant(n, L):

    E = []

    for i in range(n):

        for j in range(i+1, n):

            if(((i-j) % n) in L):

                if({i, j} not in E):

                    E.append({i, j})
    return E


# Is K critical test
def is_k_critical(G, k):

    V = G.vertices()
    chi = G.chromatic_number()

    if(chi != k):
        return False

    for v in V:

        # creates local copy of G so we can delete vertices and maintain G's structure
        H = Graph(G)
        H.delete_vertex(v)

        if vertex_coloring(H, k=k-1, value_only=True) == False:
            return False

    return True


# testing the 3 1 ... 1 3 pattern
def pattern_3113(p):

    params = []

    params.append(p)

    # Get the param values
    params = params + [i for i in range(p+3, 3*p - 1)]

    params.append(3*p + 1)

    return params


# testing the 2 1 ... 1 2 pattern
def pattern_2112(p):

    params = []

    params.append(p)

    # Get the param values
    params = params + [i for i in range(p+2, 2*p + 5)]

    params.append(4*p - 2)

    return params


# testing the 3 1 ... 1 3 pattern
def pattern_3113(p):

    params = []

    params.append(p)

    # Get the param values
    params = params + [i for i in range(p+3, 3*p - 1)]

    params.append(3*p + 1)

    return params


# testing the  1 .. 3 .. 1 pattern
def pattern_131(p):

    params = [i for i in range(p, 2*p)] + [i for i in range(2*p+2, 3*p+2)]

    return params


# P starts at 4
for p in range(3, 12 + 1):

    pattern = pattern_2112(p)

    # Show the patterns
    print(pattern)

    # Create the circulant graph
    G = Graph(circulant(4*p + 1, pattern))

    # Set the K value
    k = 5

    # Check if graph is critical
    print(f'Is {k} Critical: {is_k_critical(G, k)}')


"""
May 18, 2023
-----------------------------
GRAPH %
# Pattern 1112121111
- 5 6 7 8 10 11 13 14 15 16 (circ21_chi5)
- 6 7 8 9 10 11 14 15 16 17 18 19 (circ25_chi5)

[1111] then [2] [1] [2] and [1111]
the number of starting and ending 1's are equal to the first digit minus one [5 - 1] = 4 one's to start and end the sequence
-------------------------------
GRAPH 6
# Pattern 2 2 1 [2 2] 1 2 2

- 5 7 9 10 12 14 16 17 19 21 (circ 26 crit 6)


# Pattern 3 2 1 1 1 1 1 1 1 2 3

"""