// The first dimension of the matrix contains what node of the graph we're looking at. Length is "nv"
// The second dimension is a number that labels each connection (Connection 0, Connection 1, etc). The thing inside each array is what the 
// destination node is. Length is also "nv", but only has elements up to "adj[v]" if "v" is the node from the first dimension
// Basically, "adj[v]" is the length of the array in "graph[v]"

// So in order to find all the connections that a vertex has, we loop through 0 to adj[v] (degree of v) and find all the values


//TODO: discard all the pointers when you're done with them!

typedef unsigned char GRAPH[MAXN + 1][MAXN];
typedef unsigned char ADJACENCY[MAXN + 1];

// The graph which is currently under construction
GRAPH current_graph;
// GRAPH compliment_graph;

ADJACENCY adj;

int nv;

//Finds whether a graph is a P4 + P1 via its degree signature
static bool is_P4_P1(){

    if (nv != 5) return false;

    int timesZero;
    int timesOne;
    int timesTwo;

    for (currentNode = 0; currentNode < nv; currentNode++)
    {
        //Loop through each vertex to try and find the P4 + P1 degree signature
        if (adj[currentNode] > 2) return false;

        if (adj[currentNode] == 0){
            timesZero++;
        }
        if (adj[currentNode] == 1){
            timesOne++;
        }
        if (adj[currentNode] == 2){
            timesTwo++;
        }
    }

    if (timesZero != 1) return false;
    if (timesOne != 2) return false;
    if (timesTwo != 2) return false;

    return true;
}

//This translates vertices from a bigger graph to a smaller one
static void vertexShrinkDict(int inNV, int deleteVertex, int* outDict){

    int temp[inNV];

    int dictValue = 0;
    for (int i = 0; i < inNV; i++){
        
        if (i == deleteVertex){
            temp[i] = -1;
        }
        else{
            temp[i] = dictValue;
            dictValue++;
        }

    }

    //This copies the pointer address over
    //We don't have to discard the array because it isn't dynamic
    outDict = temp;
}

static void removeVertexFromGraph(GRAPH g, ADJACENCY inAdj, int inNV, int deleteVertex, char** outGraph, ADJACENCY* outAdj){
    //outGraph and outAdj are initialized out of the function
    //outGraph = new char[MAXN][MAXN - 1];

    //We can't remove a vertex that already exists
    if (deleteVertex >= inNV) return;

    //Create our dict array
    int* outDict;
    vertexShrinkDict(inNV, deleteVertex, outDict);

    for (int i = 0; i < inNV; i++){

        if (i == deleteVertex) continue;

        //The inner arrays are 1 shorter than the outer array because they exclude the vertex itself
        for (int j = 0; j < inNV - 1; j++){

            if (g[i][j] == deleteVertex) continue;

            //We take the value from the input graph and map it to the smaller graph size (outDict[g[i][j]])
            //Then we calculate where to put it in the output graph (outGraph[outDict[i]][outDict[j]])
            outGraph[outDict[i]][outDict[j]] = outDict[g[i][j]];

            outAdj[i]++;
        }
    }

}

//This just brute force checks all the possible ways we can remove edges and tells us if a P4 + P1 is in there someplace
static bool recursive_contains_induced_P4_P1(GRAPH g, ADJACENCY adj, int nv){
    //Base case
    if (nv == 5){
        return is_P4_P1();
    }

    for (int i = 0; i < nv; i++){

        char outGraph[nv][nv - 1];
        ADJACENCY outAdj;

        removeVertexFromGraph(g, inAdj, inNV, i, outGraph, outAdj);

        //If we find a graph with our recursion, return true
        if (recursive_contains_induced_P4_P1(outGraph, outAdj, nv - 1)) return true;

    }

    //No graphs were found
    return false;
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

