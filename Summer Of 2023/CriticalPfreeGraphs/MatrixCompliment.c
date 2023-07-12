// The first dimension of the matrix contains what node of the graph we're looking at. Length is "nv"
// The second dimension is a number that labels each connection (Connection 0, Connection 1, etc). The thing inside each array is what the 
// destination node is. Length is also "nv", but only has elements up to "adj[v]" if "v" is the node from the first dimension
// Basically, "adj[v]" is the count of "graph[v]"

// So in order to find all the connections that a vertex has, we loop through 0 to adj[v] (degree of v) and find all the values

typedef unsigned char GRAPH[MAXN + 1][MAXN];
typedef unsigned char ADJACENCY[MAXN + 1];

// The graph which is currently under construction
GRAPH current_graph;
// GRAPH compliment_graph;

ADJACENCY adj;

int nv;

// For reference
static void
add_edge_one_way(GRAPH graph, unsigned char adj[], unsigned char v, unsigned char w)
{
    graph[v][adj[v]] = w;
    // graph[w][adj[w]] = v;
    adj[v]++;
    // adj[w]++;
}


static char[] complete_dest_arr(int range) {

    //This is just all the numbers from 0 to (range - 1)
    char temp[range];
    for (int i = 0; i < range; i++){
        temp[i] = i
    }

    return temp;
}

static void arr_diff(char[] one, char[] two, int inLength, char* outArr, int* outLength) {

    int currentDiffIndex = 0;

    //Find the difference

    for (int i = 0; i < inLength; i++) {
        
        bool matchFound = false;

        for (int j = 0; j < inLength; j++){

            if (one[i] == two[j]) {
                matchFound = true;
                break;
            }

        }

        if (!matchFound) {
            //If no matches are found, this part of the input set is disjoint to the output
            outArr[currentDiffIndex] = one[i];
            currentDiffIndex++;
        }
    }

    //Output the difference length so we can use it later
    *outLength = currentDiffIndex;

    return temp;
}

static GRAPH compliment_of_current() {

    GRAPH g;

    for (currentNode = 0; currentNode < nv; currentNode++)
    {
        //Loop through each vertex
        //For each row in the new matrix, we want to find the difference between current_graph[currentNode] and complete_dest_arr(nv)

        char newRow[nv];
        int* newRowLen = malloc(1);

        arr_diff(current_graph[currentNode], complete_dest_arr(nv), nv, newRow, newRowLen);

        for (j = 0; j < *newRowLen; j++)
        {
            //For each thing in the difference, put it in the graph row
            //HOWEVER we also have to subtract the vertex itself from the difference

            if (newRow[j] != currentNode)
                g[currentNode][j] = newRow[j];
        }
    }

    return g;
}

//Illustration matrix of P4 + P1
//graph[0] = [1]
//graph[1] = [0,2]
//graph[2] = [1,3]
//graph[3] = [2]
//graph[4] = []

//adj[0] = 1
//adj[1] = 2
//adj[2] = 2
//adj[3] = 1
//adj[4] = 0

//Set of all vertices
//[0,1,2,3,4]

//Illustration matrix of gem (compliment of P4 + P1)
//Take difference of the rows in the original graph and the set of all vertices
//HOWEVER we also have to subtract the vertex itself from the difference
//graph[0] = [2,3,4]
//graph[1] = [3,4]
//graph[2] = [0,4]
//graph[3] = [0,1,4]
//graph[4] = [0,1,2,3]

//adj[0] = 3
//adj[1] = 2
//adj[2] = 2
//adj[3] = 3
//adj[4] = 4