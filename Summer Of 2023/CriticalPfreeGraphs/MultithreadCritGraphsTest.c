// The first dimension of the matrix contains what node of the graph we're looking at. Length is "nv"
// The second dimension is a number that labels each connection (Connection 0, Connection 1, etc). The thing inside each array is what the 
// destination node is. Length is also "nv", but only has elements up to "adj[v]" if "v" is the node from the first dimension
// Basically, "adj[v]" is the length of the array in "graph[v]"

// So in order to find all the connections that a vertex has, we loop through 0 to adj[v] (degree of v) and find all the values


//TODO: discard all the pointers when you're done with them!

#include <pthread.h>
#include <stdio.h>

//Constants
#define NUM_THREADS 4

typedef unsigned char GRAPH[MAXN + 1][MAXN];
typedef unsigned char ADJACENCY[MAXN + 1];

// The graph which is currently under construction
GRAPH current_graph;
// GRAPH compliment_graph;

ADJACENCY adj;

int nv;
int graphCount = 0;

//First dimension stores what graph we're looking at
//Second dimension is the code arrays
unsigned char** code2D;

void* extend_multithread(void* arg){
    int thread_id = *(int*)arg;
    int start = thread_id * (graphCount / NUM_THREADS);
    int end = (thread_id + 1) * (graphCount / NUM_THREADS);

    for(int i = start; i < end; i++) {
        char* code = code2D[i];

        decode_multicode(code, current_graph, adj, codelength);

        total_number_of_inputgraphs_read++;

        //fprintf(stderr, "Processing graph %llu with %d vertices\n", total_number_of_inputgraphs_read, nv);

        copy_nauty_graph(current_graph, adj, nv, nautyg);

        // printgraph(current_graph, adj, nv);
        // printgraph_nauty(nautyg, nv);

        extend(isolated_vertices, 0, 0, 0, 1);

    }
}

void test() {

    if (start_from_inputgraphs)
    {
        //Moved to outside function; we need to multithread this
        unsigned char code[MAXN * (MAXN - 1) / 2 + MAXN];

        unsigned long long int total_number_of_inputgraphs_read = 0;
        int nuller;
        int codelength;
        int nv_previous = -1;


        while (fread(&nv, sizeof(unsigned char), 1, stdin))
        {
            if (nv > MAXN)
            {
                fprintf(stderr, "Error: can only read graphs with at most %d vertices (tried to read graph with %d vertices)\n", MAXN, nv);
                exit(1);
            }

            if (nv > maxnv)
            {
                fprintf(stderr, "Error: inputgraph is too big: %d vs max %d\n", nv, maxnv);
                exit(1);
            }

            if (modulo > 1 && nv > splitlevel)
            {
                fprintf(stderr, "Error: graph is larger than splitlevel!\n");
                exit(1);
            }

            if (nv_previous != -1 && nv != nv_previous)
            {
                fprintf(stderr, "Error: read inputgraphs with different nv!\n");
                exit(1);
            }
            nv_previous = nv;

            code[0] = nv;
            nuller = 0;
            codelength = 1;
            while (nuller < nv - 1)
            {
                code[codelength] = getc(stdin);
                if (code[codelength] == 0)
                    nuller++;
                codelength++;
            }
            

            graphCount++;
        }

        //Initialize the first layer (length of graphCount)
        code2D = malloc(sizeof(char*) * graphCount);
        //Initialize the second layer (length of MAXN * (MAXN - 1) / 2 + MAXN)
        for(int i = 0; i < graphCount; i++){
            code2D[i] = malloc(sizeof(char*) * (MAXN * (MAXN - 1) / 2 + MAXN));
        }

        pthread_t threads[NUM_THREADS];
        int thread_ids[NUM_THREADS];

        //Create the threads
        for(int i = 0; i < NUM_THREADS; i++){
            thread_ids[i] = i;
            pthread_create(&threads[i], NULL, extend_multithread, &thread_ids[i]);
        }

        //Join the threads, going back to 1 thread processing
        for(int i = 0; i < NUM_THREADS; i++){
            pthread_join(&threads[i], NULL);
        }


        //Discard the 2D code array!
        for(int i = 0; i < graphCount; i++){
            free(code2D[i]);
        }
        free(code2D);

        fprintf(stderr, "Read %llu inputgraphs\n", total_number_of_inputgraphs_read);

        return;
    }

}
