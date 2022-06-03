from sage.all import *




# Create the circulant graph
def circulant_graph(number_of_vertices):
    
    connections = []
    
    for vertex in range(number_of_vertices):

        for new_vertex in range(number_of_vertices - 1):
            
            if vertex != new_vertex:
            
                connection = [vertex, new_vertex]
                
                connections.append(connection)
        
    return connections

cc = circulant_graph(12)
print(cc)

Graph(cc).show()