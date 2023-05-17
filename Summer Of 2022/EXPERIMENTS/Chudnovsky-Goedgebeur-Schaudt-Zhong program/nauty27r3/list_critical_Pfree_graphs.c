/**
 * list_critical_Pfree_graphs.c
 *
 * A generator for k-list-critical Pt-free graphs.
 *
 * The latest version of ListCriticalPfreeGraphs can be found here:
 * http://caagt.ugent.be/listcriticalpfree/
 * 
 * Author: Jan Goedgebeur (jan.goedgebeur@ugent.be)
 * In collaboration with Oliver Schaudt
 *
 * -----------------------------------
 * 
 * Copyright (c) 2015-2016 Ghent University
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * Compile with:
 *
 * cc -DWORDSIZE=32 -DMAXN=WORDSIZE -O4 list_critical_Pfree_graphs.c nautyW1.a -o list_critical_Pfree_graphs
 *
 * Or:
 *
 * cc -DWORDSIZE=64 -DMAXN=WORDSIZE -O4 list_critical_Pfree_graphs.c nautyL1.a -o list_critical_Pfree_graphs
 *
 * -DWORDSIZE=32 is slightly faster, but limits the order of the graphs to 32.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include "nauty.h"

#define VERSION "1.0 - December 8 2015"


/*************************Defines and global variables*************************/

/******************************Configuration***********************************/

//If the number of vertices is large enough, the splitlevel should be at least MIN_SPLITLEVEL
#define MIN_SPLITLEVEL 9
//#define MIN_SPLITLEVEL 8

#define MAX_SPLITLEVEL 20

//Uncomment to output only k-critical H-free graphs (is a bit slower)
//Else possibly also non-critical graphs might be output (so these graphs still need to be tested for criticality by afterwards)
#define TEST_IF_CRITICAL


//Could change this to 2, but must be at least 1
//#define MIN_COLOUR_SET_SIZE 2
#define MIN_COLOUR_SET_SIZE 1

/* 
 * Instead of constructing all possible paths for the P-free test, first test if one of the
 * NUM_PREV_SETS previous forbidden induced paths is still a forbidden induced path.
 * (As nearly all of the generated graphs contain a forbidden induced path)
 * 5 seems to be a good trade-off for this.
 */
#define NUM_PREV_SETS 20 //20 seems to be a good compromise for this


/***************You should not change anything below this line*****************/

//For the timing
#define time_factor sysconf(_SC_CLK_TCK) 

/*****************************Some useful macros*******************************/

#if WORDSIZE == 64
#define BIT(i) (1ULL << (i))
#else //i.e. WORDSIZE == 32
#define BIT(i) (1U << (i))
#endif

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

//Test if N(a) \subseteq N(b)
#define NBH_IS_SUBSET_OF(a, b) ((*GRAPHROW1(nautyg, (a), MAXM) & *GRAPHROW1(nautyg, (b), MAXM)) == *GRAPHROW1(nautyg, (a), MAXM))

//#define NBH_IS_SUBSET_OF_COL_DOM_ALL(a, b) ((*GRAPHROW1(nautyg, (a), MAXM) & *GRAPHROW1(nautyg_colour_dominated_all, (b), MAXM)) == *GRAPHROW1(nautyg, (a), MAXM))
#define NBH_IS_SUBSET_OF_COL_DOM_VERTEX_REMOVED(a, b) ((*GRAPHROW1(nautyg, (a), MAXM) & *GRAPHROW1(nautyg_colour_dominated_vertex_removed[a], (b), MAXM)) == *GRAPHROW1(nautyg, (a), MAXM))

//size of multicode is nv + ne        
#define MAX_MUTLICODE_SIZE(a) ((a) + (a)*((a)-1)/2)

/***********************Methods for splay-tree*****************************/

//Methods for the splay-tree (see splay.c for more information). 

typedef struct sp {
    //graph canon_form[MAXN*MAXM];
    graph* canon_form; //Is a pointer, should use malloc for this; isn't really faster but uses fewer memory!
    //graph* av_colours_canon;
    struct sp *left, *right, *parent;
} SPLAYNODE;


//SPLAYNODE *worklist = NULL;
//list_of_worklists[i] is the worklist for graphs with i vertices
SPLAYNODE *list_of_worklists[MAXN+1] = {NULL};

/* extra arguments to pass to scan procedure */
#define SCAN_ARGS

/* what scan procedure should do for each node p */
#define ACTION(p) outputnode(p)

/* extra arguments for the insertion procedure */
#define INSERT_ARGS , graph *canong, int *is_new_node

/* how to compare the key of INSERT_ARGS to the key of node p */
#define COMPARE(p) comparenodes(canong, p)

/* what to do for a new node p */
#define NOT_PRESENT(p) new_splaynode(p, canong, is_new_node)

/* what to do for an old code p */
#define PRESENT(p) old_splaynode(p, is_new_node)


void new_splaynode();
void old_splaynode();

#include "splay.c"

/******************************************************************************/

//+1 needed for minibaum representation
typedef unsigned char GRAPH[MAXN+1][MAXN];
typedef unsigned char ADJACENCY[MAXN+1];

//The graph which is currently under construction
GRAPH current_graph;
ADJACENCY adj;

GRAPH available_colours;
ADJACENCY num_available_colours;

graph available_colours_bitvector[MAXN];

GRAPH current_graph_copy;

//Minibaum graph (here g[0][0]=nv)
GRAPH current_graph_mb;
ADJACENCY adj_mb;

#define leer 255 

int nv = 0; //Nv of current_graph
int maxnv = 0;

//For nauty
#define WORKSIZE (50 * MAXM)

int lab[MAXN], ptn[MAXN], orbits[MAXN];
static DEFAULTOPTIONS_GRAPH(options);
statsblk stats;
setword workspace[WORKSIZE];

graph nautyg[MAXN*MAXM];

graph nautyg_gadget[MAXN*MAXM];

int is_sim_pair = 0;

//Colour domination graph where a given vertex is removed
graph nautyg_colour_dominated_vertex_removed[MAXN][MAXN*MAXM];

/******************************************************************************/

/* Some marks */
#include <limits.h> //Necessary for marks
unsigned int marks[MAXN];

#define MAXVAL INT_MAX - 1
static int markvalue = MAXVAL;
#define RESETMARKS {int mki; if ((markvalue += 1) > MAXVAL) \
      { markvalue = 1; for (mki=0;mki<MAXN;++mki) marks[mki]=0;}}
#define MARK(v) marks[v] = markvalue
#define UNMARK(v) marks[v] = markvalue - 1
#define ISMARKED(v) (marks[v] == markvalue)

/******************************************************************************/

//For contains_forbidden_induced_path

//Only accepts graphs which do not contain induced paths of more than max_path_size vertices
//So only outputs Pn-free graphs with n = max_path_size+1
int max_path_size = 0;

static int current_path[MAXN];
static int current_pathsize = 0;
static setword current_path_bitvector;

static int longest_pathsize_found = 0;


//Using malloc because it's safer (will exit if not enough memory can be allocated)
//Important: make sure everything is initialized to 0!
//static int previous_induced_path[MAXN+1][NUM_PREV_SETS][MAXN] = {0};
int (*previous_induced_paths)[MAXN+1][NUM_PREV_SETS][MAXN];

//For contains_forbidden_induced_cycle
int test_if_contains_forbidden_cycle = 0;
int forbidden_cycle_size = 0;

int test_if_trianglefree = 0;

int test_if_diamondfree = 0;

int test_if_bipartite = 0;

//If set to one vertex critical instead of critical graphs are output
int output_vertex_critical_graphs = 0;

//For is_critical()

typedef unsigned char EDGE[2];

#define MAX_EDGES ((MAXN)*((MAXN)-1)/2)


/******************************************************************************/

//For contains_W5 test
static unsigned long long int possible_cycle_vertices = 0;
static int contains_forbidden_cycle = 0;


/******************************************************************************/

//Only accepts graphs which are NOT k-colourable (on last level)
int num_colours = 0;

//int previous_extension_was_not_colourable = 0;

/* Specific parameters for modulo */
static int modulo = 1;
static int rest = 0;
static int splitcounter = 0;
static int splitlevel = 0; /* For modolo, determines at what level/depth of the recursion tree the calculation should be split */

/******************************************************************************/

//For statistics
static unsigned long long int num_graphs_generated[MAXN + 1];
static unsigned long long int num_graphs_generated_iso[MAXN + 1];
static unsigned long long int num_graphs_generated_noniso[MAXN + 1];
static unsigned long long int num_graphs_written = 0;

static unsigned long long int times_graph_Pfree[MAXN + 1] = {0};
static unsigned long long int times_graph_not_Pfree[MAXN + 1] = {0};

static unsigned long long int times_induced_path_found_previous[MAXN + 1] = {0};
static unsigned long long int times_induced_path_not_found_previous[MAXN + 1] = {0};

static unsigned long long int counts_contains_forbidden_induced_cycle[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_forbidden_induced_cycle[MAXN + 1] = {0};

static unsigned long long int times_graph_colourable[MAXN + 1] = {0};
static unsigned long long int times_graph_not_colourable[MAXN + 1] = {0};

static unsigned long long int times_graph_vertex_critical[MAXN + 1] = {0};
static unsigned long long int times_graph_not_vertex_critical[MAXN + 1] = {0};

static unsigned long long int times_similar_element_destroyed[MAXN+1] = {0};
static unsigned long long int times_similar_element_not_destroyed[MAXN+1] = {0};

static unsigned long long int times_contains_similar_vertices[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_vertices[MAXN+1] = {0};

static unsigned long long int times_contains_similar_vertices_col_dom[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_vertices_col_dom[MAXN+1] = {0};

static unsigned long long int times_contains_similar_edgepair[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_edgepair[MAXN+1] = {0};

static unsigned long long int times_contains_small_vertex[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_small_vertex[MAXN+1] = {0};

static unsigned long long int times_no_lemmas_applicable[MAXN+1] = {0};

/*********************************Methods**************************************/

static void extend_Pfree(setword isolated_vertices, unsigned char avoided_vertex);

static void test_if_graph_is_P_free_and_extend(setword isolated_vertices, unsigned char avoided_vertex);

/******************************************************************************/

/**
 * Prints the given nauty graph.
 */
static void
printgraph_nauty(graph *g, int current_number_of_vertices) {
    int i;
    fprintf(stderr, "Printing graph nauty:\n");
    for(i = 0; i < current_number_of_vertices; i++) {
        fprintf(stderr, "%d :", i);
        set *gv = GRAPHROW1(g, i, MAXM);
        int neighbour = -1;
        while((neighbour = nextelement(gv, MAXM, neighbour)) >= 0) {
            fprintf(stderr, " %d", neighbour);
        }
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

static void
printgraph_minibaum(GRAPH g, ADJACENCY adj, int nv) {
    int i, j;
    fprintf(stderr, "Printing graph: \n");
    for(i = 1; i <= nv; i++) {
        fprintf(stderr, "%d :", i);
        for(j = 0; j < adj[i]; j++)
            fprintf(stderr, " %d", g[i][j]);
        fprintf(stderr, "\n");
    }
}

/******************************************************************************/

static void
printgraph(GRAPH g, ADJACENCY adj, int nv) {
    int i, j;
    fprintf(stderr, "Printing graph: \n");
    for(i = 0; i < nv; i++) {
        fprintf(stderr, "%d :", i);
        for(j = 0; j < adj[i]; j++)
            fprintf(stderr, " %d", g[i][j]);
        fprintf(stderr, "\n");
    }
}

/*********************Methods for splay tree*********************************/

void new_splaynode(SPLAYNODE *el, graph *canong, int *is_new_node) {
    el->canon_form = canong;
    //el->av_colours_canon = av_cols;
    //memcpy(el->canon_form, canong, sizeof(graph) * nv);
    *is_new_node = 1;
}

/****************************************/

void old_splaynode(SPLAYNODE *el, int *is_new_node) {
    *is_new_node = 0;
    //fprintf(stderr, "Graph was already present. Nv: %d\n", nv);
}

/****************************************/

int comparenodes(graph *canong, SPLAYNODE *list) {
    //return memcmp(canong, list->canon_form, nv * sizeof (graph));
    //Important: now +3 for triangle gadget!
    return memcmp(canong, list->canon_form, (nv+3) * sizeof (graph));
    //return memcmp(canong, list->canon_form, MAXN * sizeof (graph)); //Is a lot slower!
    
/*
    int res = memcmp(canong, list->canon_form, nv * sizeof (graph));
    if(res == 0) //Graphs are isomorphic, now check if canon colouring is also isomorphic
        return memcmp(av_cols, list->av_colours_canon, nv * sizeof (graph));
    else
        return res;
*/
}

/****************************************/

outputnode(SPLAYNODE *liste) {
    fprintf(stderr, "Error: outputting of nodes not allowed\n");
    exit(1);
}

/******************************************************************************/

/* Assumes that MAXM is 1 (i.e. one_word_sets) */
//This is slightly faster...
int
nextelement1(set *set1, int m, int pos) {
    setword setwd;

    if(pos < 0) setwd = set1[0];
    else setwd = set1[0] & BITMASK(pos);

    if(setwd == 0) return -1;
    else return FIRSTBIT(setwd);
}

/******************************************************************************/

/*
 * Converts g to a graph in the nauty format and stores the resulting graph
 * in nauty_graph.
 */
static void
copy_nauty_graph(GRAPH g, ADJACENCY adj, int num_vertices, graph *nauty_graph) {
    int i, j;
    for(i = 0; i < num_vertices; i++) {
        nauty_graph[i] = 0;
        for(j = 0; j < adj[i]; j++)
            ADDELEMENT1(&nauty_graph[i], g[i][j]);
    }

}

/******************************************************************************/

/**
 * Code the graph using the multicode format and returns the length of the code.
 */
int code_multicode(unsigned char code[], GRAPH g, ADJACENCY adj, int num_of_vertices) {
    int i, j;
    int codelength = 0;
    code[codelength++] = num_of_vertices;
    for(i = 0; i < num_of_vertices - 1; i++) {
        for(j = 0; j < adj[i]; j++)
            if(g[i][j] > i)
                code[codelength++] = g[i][j] + 1;
        code[codelength++] = 0;
    }

    return codelength;
}

/******************************************************************************/

//Outputs the graph to stdout in multicode
static void
output_graph(GRAPH g, ADJACENCY adj, int num_vertices) {

    unsigned char codebuffer[MAX_MUTLICODE_SIZE(num_vertices)];
    int codelength = code_multicode(codebuffer, g, adj, num_vertices);

    if(fwrite(codebuffer, sizeof (unsigned char), codelength, stdout)
            < sizeof (unsigned char) * codelength) {
        fprintf(stderr, "Error: couldn't write graph! \n\n");
        exit(1);
    }

    num_graphs_written++;
}

/******************************************************************************/

/**
 * Adds a new vertex and connects it will all vertices of
 * the vertex set. 
 */
static void
expand_vertex_set_no_remove(unsigned char dominating_set[], int set_size) {
    nautyg[nv] = 0;
    int i;
    for(i = 0; i < set_size; i++) {
        current_graph[dominating_set[i]][adj[dominating_set[i]]++] = nv;
        current_graph[nv][i] = dominating_set[i];
        ADDELEMENT1(&nautyg[dominating_set[i]], nv);
        ADDELEMENT1(&nautyg[nv], dominating_set[i]);
    }
    adj[nv] = set_size;

    nv++;
}

/******************************************************************************/

/**
 * Reduction of expand_dominating_set_no_remove()
 */
static void
reduce_vertex_set_no_remove(unsigned char dominating_set[], int set_size) {
    nv--;

    int i;
    for(i = 0; i < set_size; i++) {
        adj[dominating_set[i]]--;
        DELELEMENT1(&nautyg[dominating_set[i]], nv);
    }

}

/******************************************************************************/

//Returns 1 if the graph contains a triangle which also contains the last vertex, else returns 0
//The last vertex must be in the triangle, since the parents were triangle-free
//Important: it is assumed that the vertices of the last vertex are current_set_bitvector
static int
contains_triangle_without_expand(unsigned char current_set[], int current_set_size, 
        setword current_set_bitvector) {
/*
    //set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(&current_set_bitvector, 1, neighbour_b1)) >= 0) {
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & current_set_bitvector;
        if(intersect_neigh_n_b1 != 0)
            return 1;
    }
*/
    //The method below is a lot faster!
    int i;
    for(i = 0; i < current_set_size; i++)
        if((nautyg[current_set[i]] & current_set_bitvector) != 0)
            return 1;
    
    return 0;
}

/******************************************************************************/

static void store_current_path() {
    //Store as queue
    int i;
    for(i = NUM_PREV_SETS - 1; i > 0; i--)
        memcpy((*previous_induced_paths)[nv][i], (*previous_induced_paths)[nv][i - 1], sizeof (int) * current_pathsize);

    //Not really any difference if using current_pathsize or MAXN for memcpy
    memcpy((*previous_induced_paths)[nv][0], current_path, sizeof (int) * current_pathsize);

    //memcpy(previous_induced_path[nv], current_path, sizeof(int) * current_pathsize);
}

/******************************************************************************/

//It is assumed that current_pathlength > 0
static void
determine_longest_induced_path_recursion(int maxlength) {
    if(current_pathsize > longest_pathsize_found) {
        longest_pathsize_found = current_pathsize;
        if(longest_pathsize_found > maxlength) {
            store_current_path();
            
            return;
        }
    }

    int previous_vertex = current_path[current_pathsize - 1];
    setword previous_vertex_bitvector = 0;
    ADDELEMENT1(&previous_vertex_bitvector, previous_vertex);

    //set *gv = GRAPHROW1(nautyg, previous_vertex, MAXM);
    //Is a lot faster when using current_graph instead of nextelement1!
    //while((neighbour = nextelement1(gv, 1, neighbour)) >= 0) {
    int i;
    for(i = 0; i < adj[previous_vertex]; i++) {
        int neighbour = current_graph[previous_vertex][i];
        set *gv_neighbour = GRAPHROW1(nautyg, neighbour, MAXM);
        if((current_path_bitvector & *gv_neighbour) == previous_vertex_bitvector //Is the bottleneck but can't seem to get this faster?
                && neighbour != current_path[0]) {
            //&& (BIT(next_vertex) & current_path_bitvector) == 0) {
            //&& !ISMARKED(next_vertex)) {
            current_path[current_pathsize++] = neighbour;
            ADDELEMENT1(&current_path_bitvector, neighbour);

            //recursion
            determine_longest_induced_path_recursion(maxlength);

            //Is slightly faster with this included
            if(longest_pathsize_found > maxlength)
                return;

            current_pathsize--;
            DELELEMENT1(&current_path_bitvector, neighbour);
        }

    }

}

/******************************************************************************/

/**
 * Returns 1 if the vertices in possible_induced_path[] form an induced path of
 * with maxlength + 1 vertices, else returns 0.
 */
static int 
previous_induced_path_is_still_induced(int possible_induced_path[], int maxlength) {
    int i;
    setword current_path_bitvector = 0;
    for(i = 0; i <= maxlength; i++) {
        int next_vertex = possible_induced_path[i];
        //if(next_vertex == -1) //Everything will be initialised to 0, so that's fine!
        //    return 0;
        setword previous_vertex_bitvector = 0;
        if(i > 0)
            ADDELEMENT1(&previous_vertex_bitvector, possible_induced_path[i-1]);
        set *gv_next_vertex = GRAPHROW1(nautyg, next_vertex, MAXM);
        if(i != 0 && (current_path_bitvector & *gv_next_vertex) != previous_vertex_bitvector)
            return 0;
        
        ADDELEMENT1(&current_path_bitvector, next_vertex);
    }
    
    return 1;
}

/******************************************************************************/

//Returns 1 if current_graph contains a forbidden induced path, else returns 0
static int
contains_forbidden_induced_path() {
    //Note: nearly every graph contains a forbidden induced path
    //so first try some cheap heuristics to find such a path quickly

    //First check if previous path is perhaps still an induced path
    int i;
        for(i = 0; i < NUM_PREV_SETS; i++)
            if(previous_induced_path_is_still_induced((*previous_induced_paths)[nv][i], max_path_size)) {
                times_induced_path_found_previous[nv]++;
                return 1;
            }
        times_induced_path_not_found_previous[nv]++;
    
    //Only apply expensive exhaustive algorithm when greedy approach didn't find any induced paths!    

    //Note: only testing the last vertex is not sufficient even though we know the
    //parent is P-free
    longest_pathsize_found = 0;
    for(i = 0; i < nv; i++) {
        //for(i = n-1; i >= 0; i--) { //doesn't make any difference
        current_path[0] = i;
        current_pathsize = 1;
        current_path_bitvector = 0;
        ADDELEMENT1(&current_path_bitvector, i);

        //recursion
        determine_longest_induced_path_recursion(max_path_size);
        if(longest_pathsize_found > max_path_size)
            return 1; //prune
    }
    return 0;
}

/******************************************************************************/

//It is assumed that current_pathlength > 0
static void
contains_forbidden_induced_cycle_recursion(int forbiddencyclesize) {
    if(current_pathsize >= forbiddencyclesize)
        return;
    
    int previous_vertex = current_path[current_pathsize-1];
    int i;
    for(i = 0; i < adj[previous_vertex]; i++) {
        int next_vertex = current_graph[previous_vertex][i];

        if(ISELEMENT1(&possible_cycle_vertices, next_vertex)) { //marks seem to be faster than bv here
        //if(ISELEMENT1(&possible_cycle_vertices, next_vertex) && !ISMARKED(next_vertex)) { //marks seem to be faster than bv here
        //if((BIT(next_vertex) & current_path_bitvector) == 0) {
            if(current_pathsize < forbiddencyclesize - 1 
                    //Doesn't have to be an induced cycle, but it is best to be as restrictive as possible as usually it does not contain a C5
                    && (*GRAPHROW1(nautyg, next_vertex, MAXM) & current_path_bitvector) == BITT[previous_vertex]) {
                    //(neighbourhood[next_vertex] & current_path_bitvector) == BIT(previous_vertex)) {
                //MARK(next_vertex);
                DELELEMENT1(&possible_cycle_vertices, next_vertex);
                current_path[current_pathsize++] = next_vertex;
                //current_path_bitvector |= BIT(next_vertex);
                ADDELEMENT1(&current_path_bitvector, next_vertex);

                //recursion
                contains_forbidden_induced_cycle_recursion(forbiddencyclesize);
                if(contains_forbidden_cycle)
                    return;                

                //UNMARK(next_vertex);
                ADDELEMENT1(&possible_cycle_vertices, next_vertex);
                current_pathsize--;
                //current_path_bitvector &= ~BIT(next_vertex);
                DELELEMENT1(&current_path_bitvector, next_vertex);
            } else if(current_pathsize == forbiddencyclesize - 1 &&
                    //(neighbourhood[next_vertex] & current_path_bitvector) 
                    //== (BIT(previous_vertex) | BIT(current_path[0]))) {
                    //Doesn't have to be an induced cycle, but it is best to be as restrictive as possible as usually it does not contain a C5
                    //(*GRAPHROW1(nautyg, next_vertex, MAXM) & BITT[current_path[0]]) != 0) { //Moet zelfs niet induced zijn!
                    (*GRAPHROW1(nautyg, next_vertex, MAXM) & current_path_bitvector) 
                    == (BITT[previous_vertex] | BITT[current_path[0]])) {
                //Induced cycle formed of size current_pathsize + 1
                //fprintf(stderr, "cycle found of size %d\n", current_pathsize + 1);
                if(current_pathsize + 1 == forbiddencyclesize) {
                    contains_forbidden_cycle = 1;
                    return;
                }
                
            }
        }
    }
}

/******************************************************************************/

/**
 * Returns 1 if the graph contains an induced cycle of size forbiddencyclesize,
 * else returns 0.
 */
//Since the parents were C-free, the last vertex must be in the forbidden cycle
static int
contains_forbidden_induced_cycle(int forbiddencyclesize) {
    contains_forbidden_cycle = 0;
    
    //possible_cycle_vertices = ALLMASK(nv);
    possible_cycle_vertices = ALLMASK(nv - 1);
    current_path[0] = nv - 1;
    //MARK(i);
    current_pathsize = 1;
    //current_path_bitvector = BIT(i);
    current_path_bitvector = 0;
    ADDELEMENT1(&current_path_bitvector, nv - 1);
    //DELELEMENT1(&possible_cycle_vertices, nv - 1);        
    contains_forbidden_induced_cycle_recursion(forbiddencyclesize);

    return contains_forbidden_cycle;
}

/******************************************************************************/

//In case of similar vertices: num possible expansions = 2^(n-2)
static void
construct_and_expand_vertex_sets_forbidden_vertices(unsigned char current_set[], int current_set_size, 
        setword current_set_bitvector, int next_vertex, setword forbidden_vertices,
        setword isolated_vertices, unsigned char avoided_vertex) {
    if(current_set_size > 0) {
        //Important: can't store all vertexsets since out of mem at some point
        //+ it is a lot faster not to compute the vertexset orbits

        //Still need to test this since startvertices may already contain a C3!
        
        //Is a lot faster in the girth case to first test if the expansion won't yield a triangle
        //Since in this case expand_vertex_set_no_remove() used to be a bottleneck
        if(test_if_trianglefree && contains_triangle_without_expand(current_set, current_set_size, current_set_bitvector)) {
            //Larger expansions will certainly also contain a triangle!
            //fprintf(stderr, "triangle found!\n");
            return;
        }

        //So perform expansion here
        expand_vertex_set_no_remove(current_set, current_set_size);

        //extend_Pfree();
        test_if_graph_is_P_free_and_extend(isolated_vertices, avoided_vertex);
        
        //reduce
        reduce_vertex_set_no_remove(current_set, current_set_size);
  
        //Not anymore since we are testing all possible colourings
/*
        if(previous_extension_was_not_colourable) {
            //So the next graph also won't be colourable because it contains the same edges
            //But this graph can't be (edge)-critical since it contains more edges
            //So we can already stop here
            
            //fprintf(stderr, "nv: %d, size: %d\n", nv, current_set_size);
            
            previous_extension_was_not_colourable = 0;
            
            return;
        }
*/
    }
    
    //Make other possible sets
    int i;
    for(i = next_vertex; i < nv; i++) {
        //TODO: probably more efficient way to do this (e.g. marks or bitvector)
        //But is no bottleneck
        if(!ISELEMENT1(&forbidden_vertices, i)) {
            //if(i != forbidden_vertex1 && i != forbidden_vertex2) {
            current_set[current_set_size] = i;
            ADDELEMENT1(&current_set_bitvector, i);
            
            setword forbidden_vertices_backup;
            if(test_if_trianglefree) {
                forbidden_vertices_backup = forbidden_vertices;
                //All neighbours of the current vertex are forbidden else there will be a triangle!
                forbidden_vertices |= nautyg[i];
            }
            
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, current_set_size + 1, current_set_bitvector,
                    i + 1, forbidden_vertices, isolated_vertices, avoided_vertex);
            
            DELELEMENT1(&current_set_bitvector, i);
            
            if(test_if_trianglefree) {
                forbidden_vertices = forbidden_vertices_backup;
                //forbidden_vertices &= ~nautyg[i]; //Is slower!
            }
        }
    }

}

/******************************************************************************/

void colournext_list_colouring(GRAPH graph, int *colours, int tiefe, unsigned int forbidden[],
        int *nofc, unsigned int *MASK, int *colouring_found) {
    int changed[MAXN + 1];
    /* gibt an, ob die nofc und forbidden werte geaendert wurden */
    int maxforbidden, vertex, i, colour, nachbar;

    //for (i=2; colours[i]; i++); /* suche ersten nicht gefaerbten Wert */
    for(i = 1; colours[i]; i++); //Now must start from 1, since first vertex may not be coloured already by chromatic_number_same_colour_vertices()
    maxforbidden = nofc[i];
    vertex = i;
    for(; (i <= graph[0][0]); i++)
        if((nofc[i] > maxforbidden) && !colours[i]) {
            maxforbidden = nofc[i];
            vertex = i;
        }
    /* suche Wert mit maximaler Anzahl von verbotenen Farben, der noch nicht
       gefaerbt ist */

    //Use up to num_colours colours
    for(colour = 1; colour <= num_colours && !(*colouring_found); colour++) {
        if(!(forbidden[vertex] & MASK[colour])) {
            colours[vertex] = colour;
            for(i = 0; (nachbar = graph[vertex][i]) != leer; i++)
                if(!(MASK[colour] & forbidden[nachbar])) {
                    nofc[nachbar]++;
                    forbidden[nachbar] |= MASK[colour];
                    changed[nachbar] = 1;
                } else changed[nachbar] = 0;
            if(tiefe == graph[0][0])
                *colouring_found = 1;
            else /* d.h. tiefe < graph[0][0] */
                colournext_list_colouring(graph, colours, tiefe + 1, forbidden, nofc, MASK, colouring_found);
            colours[vertex] = 0;
            for(i = 0; (nachbar = graph[vertex][i]) != leer; i++)
                if(changed[nachbar]) {
                    nofc[nachbar]--;
                    forbidden[nachbar] &= ~MASK[colour];
                }
        }
    }
}

/******************************************************************************/

int colour_is_available(int col, int vertex) {
    int i;
    for(i = 0; i < num_available_colours[vertex]; i++)
        if(available_colours[vertex][i] == col)
            return 1;
    
    return 0;
}

/******************************************************************************/

int is_colourable_list_colouring_mb(GRAPH graph) {
    unsigned int MASK[32];
    unsigned int forbidden[MAXN + 1]; /*Die verbotenen Farben als int-menge*/
    int nofc[MAXN + 1]; /* number of forbidden colours */
    int colours[MAXN + 1];
    int i;

    MASK[0] = 1;
    for(i = 1; i < 32; i++) MASK[i] = MASK[i - 1] << 1;


    for(i = 1; i <= graph[0][0]; i++) {
        forbidden[i] = nofc[i] = colours[i] = 0;
    }

    int col;
    for(col = 1; col <= num_colours; col++)
        for(i = 1; i <= graph[0][0]; i++)
            if(!colour_is_available(col, i - 1)) {
                nofc[i]++;
                forbidden[i] |= MASK[col];
            }

    for(i = 1; i <= graph[0][0]; i++)
        if(nofc[i] >= num_colours) {
            fprintf(stderr, "Error: too much forbidden colours!\n");
            exit(1);
        }

    int colouring_found = 0;

    //colournext(graph,colours,3,&minsofar,2,forbidden,nofc,MASK,genug,&good_enough);
    //colournext(graph,colours,num_vertices_already_coloured+1,&minsofar,1,forbidden,nofc,MASK,genug,&good_enough);    
    colournext_list_colouring(graph, colours, 1, forbidden, nofc, MASK, &colouring_found);

    return colouring_found;

}

/******************************************************************************/

//Important: it is assumed that the vertices same_colour_vertices can indeed all have the same colour col_given!
int is_colourable_list_colouring_mb_same_colour_vertices(GRAPH graph, setword same_colour_vertices, 
        int col_given) {
    unsigned int MASK[32];
    unsigned int forbidden[MAXN + 1]; /*Die verbotenen Farben als int-menge*/
    int nofc[MAXN + 1]; /* number of forbidden colours */
    int colours[MAXN + 1];
    int i;

    MASK[0] = 1;
    for(i = 1; i < 32; i++) MASK[i] = MASK[i - 1] << 1;


    for(i = 1; i <= graph[0][0]; i++) {
        forbidden[i] = nofc[i] = colours[i] = 0;
    }

    int col;
    for(col = 1; col <= num_colours; col++)
        for(i = 1; i <= graph[0][0]; i++)
            if(!colour_is_available(col, i - 1)) {
                nofc[i]++;
                forbidden[i] |= MASK[col];
            }

    for(i = 1; i <= graph[0][0]; i++)
        if(nofc[i] >= num_colours) {
            fprintf(stderr, "Error: too much forbidden colours!\n");
            exit(1);
        }
    

    //Give same_colour_vertices colour col
    int j, nachbar;
    int num_vertices_already_coloured = 0;
    for(i = 1; i <= graph[0][0]; i++) {
        if(ISELEMENT1(&same_colour_vertices, i - 1)) {
            //Since colour of removed vertex might not be an available colour, so don't test!
/*
            if(!colour_is_available(col_given, i - 1)) {
                fprintf(stderr, "Error: %d was not an available colour for the vertex %d!\n", col_given, i - 1);
                exit(1);
            }
*/
            
            colours[i] = col_given;
            num_vertices_already_coloured++;
            for(j = 0; (nachbar = graph[i][j]) != leer; j++) {
                if(!(MASK[col_given] & forbidden[nachbar])) { //Since same colour vertices may have same neighbour
                    nofc[nachbar]++;
                    forbidden[nachbar] |= MASK[col_given];
                }
            }
        }
    }

    //Necessary for is_valid_crown() test since all vertices might already be coloured!
    if(num_vertices_already_coloured == graph[0][0])
        return 1;    

    int colouring_found = 0;

    //colournext(graph,colours,3,&minsofar,2,forbidden,nofc,MASK,genug,&good_enough);
    //colournext(graph,colours,num_vertices_already_coloured+1,&minsofar,1,forbidden,nofc,MASK,genug,&good_enough);    
    
    //colournext_list_colouring(graph, colours, 1, forbidden, nofc, MASK, &colouring_found);

    colournext_list_colouring(graph, colours, num_vertices_already_coloured + 1,
            forbidden, nofc, MASK, &colouring_found);

    return colouring_found;

}

/******************************************************************************/

static void
make_minibaum_graph() {
    //First make minibaum format graph
    
    //Is (slightly) faster when just making current_graph again than doing it once
    //and adapting determine_longest_induced_path_recursion() to use the n+1 repres
    current_graph_mb[0][0] = nv;
    int i, j;
    for(i = 1; i <= nv; i++) {
        adj_mb[i] = adj[i-1];
        for(j = 0; j < adj[i-1]; j++)
            current_graph_mb[i][j] = current_graph[i-1][j] + 1;

        //Is necessary as it is used in the colouring routines
        current_graph_mb[i][j] = leer;
    }       
}

/******************************************************************************/

int is_colourable_list_colouring() {
    
    //First make minibaum format graph
    
    make_minibaum_graph();
    
    //printgraph_minibaum(current_graph_mb, adj_mb, nv);

    
    return is_colourable_list_colouring_mb(current_graph_mb);
}

/******************************************************************************/

static void
add_edge_one_way(GRAPH graph, unsigned char adj[], unsigned char v, unsigned char w) {
    graph[v][adj[v]] = w;
    //graph[w][adj[w]] = v;
    adj[v]++;
    //adj[w]++;
}

/******************************************************************************/

//Returns 1 if the graph is list-vertex-critical else returns 0
//A graph is list-vertex-critical if both conditions are filled:
//- you can't remove any vertices without making the graph colourable
//- you can't add any colours to the availalble colours without making the graph colourable
//Important 1: it is assumed that the graph is NOT k-colourable
//Important 2: we don't need to remove the last vertex since we know the parent is k-colourable
static int
is_list_vertex_critical() {
    //Remark: this method is not really a bottleneck
    
    //If the graph has a vertex of degree less than k, it cannot be (k+1)-critical
    //Here k == num_colours
    int i;
    //Won't help much!
/*
        for(i = 0; i < nv; i++)
            if(adj[i] < num_colours && num_available_colours[i] == num_colours) {
                //fprintf(stderr, "reject degree!\n");
                return 0;
            }
     */

    //Backup available colours
    GRAPH available_colours_backup;
    ADJACENCY num_available_colours_backup;
    //Yes, is ok because arraysizes given at compiletime!
    memcpy(available_colours_backup, available_colours, sizeof (unsigned char) * nv * MAXN);
    memcpy(num_available_colours_backup, num_available_colours, sizeof (unsigned char) * nv);

    /* Step 1: verify that we can't remove any vertex without making the graph colourable */
    
    int j, k;
    current_graph_mb[0][0] = nv - 1;
    //No need to remove the last vertex
    for(i = 0; i < nv - 1; i++) {
        //Make graph (in minibaum format) where vertex i has been removed
        for(j = 1; j <= nv - 1; j++)
            adj_mb[j] = 0;

        unsigned char current_vertex, neighbour;
        for(k = 0; k < nv; k++) {
            current_vertex = k;
            if(current_vertex != i) {
                if(current_vertex > i)
                    current_vertex--;

                for(j = 0; j < num_available_colours[k]; j++)
                    available_colours[current_vertex][j] = available_colours[k][j];
                num_available_colours[current_vertex] = num_available_colours[k];

                for(j = 0; j < adj[k]; j++) {
                    neighbour = current_graph[k][j];
                    if(neighbour != i) {
                        if(neighbour > i)
                            neighbour--;
                        add_edge_one_way(current_graph_mb, adj_mb, current_vertex + 1, neighbour + 1);
                    }
                }
            }
        }

        //Is necessary as it is used in the colouring routines
        for(j = 1; j <= nv - 1; j++)
            current_graph_mb[j][adj_mb[j]] = leer;

        //fprintf(stderr, "org graph:\n");
        //printgraph(current_graph, adj, nv);

        //fprintf(stderr, "graph after removing vertex %d\n", i);
        //printgraph(current_graph_mb, adj_mb, nv);

        int is_col = is_colourable_list_colouring_mb(current_graph_mb);

        //Restore available colours
        memcpy(available_colours, available_colours_backup, sizeof (unsigned char) * nv * MAXN);
        memcpy(num_available_colours, num_available_colours_backup, sizeof (unsigned char) * nv);

        if(!is_col) {
            //i.e. is not vertex-critical
            
            return 0;
        }
    }
    
    /* Step 2: verify that we can't add any colour without making the graph colourable */
    
    //TODO: don't try to add colours for now!
    
/*
    make_minibaum_graph();
    for(i = 0; i < nv; i++)
        if(num_available_colours[i] < num_colours) { //Else can't add any colours
            int col;
            for(col = 1; col <= num_colours; col ++)
                if(!colour_is_available(col, i)) {
                    //Try to add a colour
                    available_colours[i][num_available_colours[i]] = col;
                    num_available_colours[i]++;
                    
                    int is_col = is_colourable_list_colouring_mb(current_graph_mb);
                    num_available_colours[i]--;
                    
                    if(!is_col) {
                        //i.e. can add a colour to the lists and it's still uncolourable
                        //So the current list assignment is not critical
                        return 0;
                    }
                }
        }
*/

    return 1;
}

/******************************************************************************/

static void
remove_edge_from_neighbourhood(unsigned char vertex, unsigned char removed_vertex) {
    DELELEMENT1(&nautyg[vertex], removed_vertex);
    
    int i;
    for(i = 0; i < adj[vertex]; i++)
        if(current_graph[vertex][i] == removed_vertex)
            break;
    if(i < adj[vertex]) {
        i++;
        while(i < adj[vertex]) {
            current_graph[vertex][i-1] = current_graph[vertex][i];
            i++;
        }
        adj[vertex]--;
    } else {
        fprintf(stderr, "Error: vertex which is going to be removed was not a neighbour!\n");
        exit(1);
    }    
}

/******************************************************************************/

static void 
add_vertex_to_neighbourhood(unsigned char vertex, unsigned char new_neighbour) {
    ADDELEMENT1(&nautyg[vertex], new_neighbour);
    
    int i;
    for(i = 0; i < adj[vertex]; i++)
        if(new_neighbour < current_graph[vertex][i])
            break;
    
    int j;
    for(j = adj[vertex]; j > i ;j--)
        current_graph[vertex][j] = current_graph[vertex][j-1];
    current_graph[vertex][j] = new_neighbour;
    adj[vertex]++;
}

/******************************************************************************/

#define MAX_EDGES ((MAXN)*((MAXN)-1)/2)

static EDGE edgelist[MAX_EDGES];
static int num_edges = 0;

static EDGE edgelist_temp[MAX_EDGES];
static int num_edges_temp = 0;
static int is_colourable_edge_removed[MAX_EDGES];

static int is_critical = 0;

/******************************************************************************/

/**
 * Returns 1 if the graph contains an induced cycle of size forbiddencyclesize,
 * else returns 0.
 */
//Testing all vertices, not just the last one!
static int
contains_forbidden_induced_cycle_all(int forbiddencyclesize) {
    contains_forbidden_cycle = 0;
    
    int i;
    for(i = 0; i < nv; i++) {
        possible_cycle_vertices = ALLMASK(nv);
        
        current_path[0] = i;
        current_pathsize = 1;
        current_path_bitvector = 0;
        ADDELEMENT1(&current_path_bitvector, i);
        
        DELELEMENT1(&possible_cycle_vertices, i);
        
        contains_forbidden_induced_cycle_recursion(forbiddencyclesize);
        
        if(contains_forbidden_cycle)
            return 1;        
        
    }

    return 0;
}

/******************************************************************************/

static void remove_edges_in_all_possible_ways(int edge_index, int num_edges_removed) {
    
    if(!is_critical)
        return;

    if(num_edges_removed > 0) {
        //Can make the program a little faster by updating mb graph dynamically!

        if(is_colourable_list_colouring()) {
            //Graph is colourable, so will also be colourable if removing even more edges!
            return;
        } else {
            //i.e. graph is not colourable
            //Now test if it is P-free
            if((!test_if_contains_forbidden_cycle || !contains_forbidden_induced_cycle_all(forbidden_cycle_size))
                    && !contains_forbidden_induced_path()) {
                is_critical = 0;
                return;
            }
            
            
            //if uncol but not P-free: continue!
        }

        //This is way too slow!
/*
        if(!contains_forbidden_induced_path()) {
            //Graph - e is still P-free

            //fprintf(stderr, "graph with removed edge is still P-free\n");
            
            //printgraph(graph, adj);

            //Test if colourable
            //copy_graph_to_minibaum_format();

            //Not critical if Graph -e was also uncolourable!
            if(!is_colourable()) {
                is_critical = 0;
                return;
            }

        }
*/
    }

    int i;
    for(i = edge_index; i < num_edges; i++) {
        //remove edge i
        remove_edge_from_neighbourhood(edgelist[i][0], edgelist[i][1]);
        remove_edge_from_neighbourhood(edgelist[i][1], edgelist[i][0]);
        
        //fprintf(stderr, "removed edge %d %d\n", edgelist[i][0], edgelist[i][1]);

        //recursion
        remove_edges_in_all_possible_ways(i + 1, num_edges_removed + 1);

        //Restore
        add_vertex_to_neighbourhood(edgelist[i][0], edgelist[i][1]);
        add_vertex_to_neighbourhood(edgelist[i][1], edgelist[i][0]);

        if(!is_critical)
            return;        
    }    
    
}

/**
 * Returns 1 if the graph is edge-critical, else returns 0.
 * 
 * Important: this method is expensive, so it is important for the efficiency to
 * test vertex-criticality first.
 */
static int
is_edge_critical() {
    
    num_edges_temp = 0;
    int i, j;
    for(i = 0; i < nv; i++) {
        for(j = 0; j < adj[i]; j++) {
            current_graph_copy[i][j] = current_graph[i][j]; //Copy since order of neighbours might change!
            if(i < current_graph[i][j]) {
                edgelist_temp[num_edges_temp][0] = i;
                edgelist_temp[num_edges_temp][1] = current_graph[i][j];
                num_edges_temp++;
            }
        }
    }   
    
    num_edges = 0;
    
    //Important: have to save it and can't compute on the fly since add_vertex_to_neighbourhood
    //can shift the order of the neighbours!
    
    //First add "colourable" edges:
    for(i = 0; i < num_edges_temp; i++) {
        remove_edge_from_neighbourhood(edgelist_temp[i][0], edgelist_temp[i][1]);
        remove_edge_from_neighbourhood(edgelist_temp[i][1], edgelist_temp[i][0]);
        
        //int is_col = is_colourable();
        int is_col = is_colourable_list_colouring();

        //Restore
        add_vertex_to_neighbourhood(edgelist_temp[i][0], edgelist_temp[i][1]);
        add_vertex_to_neighbourhood(edgelist_temp[i][1], edgelist_temp[i][0]);        
        
        is_colourable_edge_removed[i] = is_col;
        
        if(is_col) {
            edgelist[num_edges][0] = edgelist_temp[i][0];
            edgelist[num_edges][1] = edgelist_temp[i][1];
            num_edges++;
        }
    }
    
    //Now add uncolourable edges
    for(i = 0; i < num_edges_temp; i++) {
        if(!is_colourable_edge_removed[i] ) {
            edgelist[num_edges][0] = edgelist_temp[i][0];
            edgelist[num_edges][1] = edgelist_temp[i][1];
            num_edges++;
        }
    }    
    
    //fprintf(stderr, "num edges: %d\n", num_edges);
    if(num_edges != num_edges_temp) {
        fprintf(stderr, "Error: num_edges != num_edges_temp\n");
        exit(1);
    }
    
    //Try to remove every edge once
    is_critical = 1;
    
    remove_edges_in_all_possible_ways(0, 0);
    
    //Restore copy since order of neighbours might have changed
    for(i = 0; i < nv; i++)
        for(j = 0; j < adj[i]; j++)
            current_graph[i][j] = current_graph_copy[i][j];
    
    return is_critical;
}

/******************************************************************************/

//Returns 1 if the graph contains similar vertices, else returns 0.

//Two vertices v,w are similar if N(v) \subseteq N(w) and L(v) \superseteq L(w)

//If 1 is returned, best_small_similar_vertex points to the *best* similar vertex which should be destroyed in the next step
//and other_larger_similar_vertex is the vertex which has a larger (or same) neighbourhood than best_small_similar_vertex

//TODO:
//The best similar vertex is the one with the largest degree (is significantly faster than taking a random one)
static int contains_similar_vertices(unsigned char *best_small_similar_vertex, unsigned char *other_larger_similar_vertex) {
    //unsigned char degree_current_best_small = 0; //Only connected graphs, so min deg will be > 0
    unsigned char degree_current_best_small = MAXN; //Min or max deg doesnt seem to make much difference here
    unsigned char degree_current_best_large = 0;
    //Min degree seems to be slightly better
    unsigned char current_best_small;
    unsigned char current_best_large;
    
    int i, j;
    for(i = 0; i < nv; i++)
        for(j = 0; j < nv; j++)
            if(i != j && NBH_IS_SUBSET_OF(i, j)
                    && (available_colours_bitvector[i] & available_colours_bitvector[j]) == available_colours_bitvector[j]) {
                //N(i) \subseteq N(j)
                //And L(i) \superseteq L(j)
                
                if(adj[i] < degree_current_best_small) {
                    degree_current_best_small = adj[i];
                    current_best_small = i;
                    current_best_large = j;
                    
                } else if(adj[i] == degree_current_best_small 
                        && adj[j] > degree_current_best_large) { //This helps a little bit
                    degree_current_best_large = adj[j];
                    current_best_small = i;
                    current_best_large = j;                    
                }
/*
                *best_small_similar_vertex = i;
                *other_larger_similar_vertex = j;
                return 1; 
*/
            }
    if(degree_current_best_small < MAXN) {
        *best_small_similar_vertex = current_best_small;
        *other_larger_similar_vertex = current_best_large;     
        
        return 1;
    } else
        return 0;    
    
}

/******************************************************************************/

//Returns 1 if the graph contains similar vertices (i.e. if there are 2 vertices u,v for which N(u) \seteq N(v))
//else returns 0.
//If 1 is returned, best_small_similar_vertex points to the *best* similar vertex which should be destroyed in the next step
//and other_larger_similar_vertex is the vertex which has a larger (or same) neighbourhood than best_small_similar_vertex
//The best similar vertex is the one with the largest degree (is significantly faster than taking a random one)
static int contains_similar_vertices_col_dom(unsigned char *best_small_similar_vertex, unsigned char *other_larger_similar_vertex) {
    //unsigned char degree_current_best_small = 0; //Only connected graphs, so min deg will be > 0
    unsigned char degree_current_best_small = MAXN;
    unsigned char degree_current_best_large = 0;
    unsigned char current_best_small;
    unsigned char current_best_large;
    
    int i, j;
    for(i = 0; i < nv; i++)
        for(j = 0; j < nv; j++)
            if(i != j && NBH_IS_SUBSET_OF_COL_DOM_VERTEX_REMOVED(i, j)
                    && (available_colours_bitvector[i] & available_colours_bitvector[j]) == available_colours_bitvector[j]) {
                //neighbourhood of i is contained in neighbourhood of j
                //And L(i) \superseteq L(j)
                
                //Choose best one!
                if(adj[i] < degree_current_best_small) {
                    degree_current_best_small = adj[i];
                    current_best_small = i;
                    current_best_large = j;
                } else if(adj[i] == degree_current_best_small 
                        && adj[j] > degree_current_best_large) { //This helps a little bit
                    degree_current_best_large = adj[j];
                    current_best_small = i;
                    current_best_large = j;                    
                }
/*
                *best_small_similar_vertex = i;
                *other_larger_similar_vertex = j;
                return 1; 
*/
            }
    if(degree_current_best_small < MAXN) {
        *best_small_similar_vertex = current_best_small;
        *other_larger_similar_vertex = current_best_large;     
        
        return 1;
    } else
        return 0;    
    
}

/******************************************************************************/

/**
 * Makes the gadget graph by adding a special triangle where the vertices represent
 * colours and adding edges between the vertices of the original graph and the triangle
 * if the corresponding colour is present as available colour for that vertex.
 * 
 * Important: it is assumed that nautyg is valid!
 */
static void
make_nauty_gadget_graph() {
    if(num_colours != 3) {
        fprintf(stderr, "Error: only 3-colourability supported for now!\n");
        //Else must add K4 instead of K3!
        exit(1);
    }    
    
    memcpy(nautyg_gadget, nautyg, sizeof (graph) * nv);
    
    
    //Original vertices and the special triangle are in different partitions
    options.defaultptn = FALSE;
    
    //Order within the partition is not important!
    int i;
    for(i = 0; i < nv; i++) {
        lab[i] = i;
        ptn[i] = 1;
    }
    ptn[nv - 1] = 0;
    
    //Vertices with labels nv, nv+1 and nv+2 are the gadget triangle
    
    for(i = nv; i < nv+3; i++) {
        lab[i] = i;
        ptn[i] = 1;
        nautyg_gadget[i] = 0;        
    }
    ptn[nv + 2] = 0;

    //Hoeft eigenlijk zelfs niet? 3 isolated vertices in zelfde partitie is eigenlijk genoeg!
/*
    ADDELEMENT1(&nautyg_gadget[nv], nv + 1);
    ADDELEMENT1(&nautyg_gadget[nv], nv + 2);

    ADDELEMENT1(&nautyg_gadget[nv + 1], nv);
    ADDELEMENT1(&nautyg_gadget[nv + 1], nv + 2);

    ADDELEMENT1(&nautyg_gadget[nv + 2], nv);
    ADDELEMENT1(&nautyg_gadget[nv + 2], nv + 1);    
*/
    
    //Now add edges
    //nv+0 is colour 1
    //nv+1 is colour 2
    //nv+2 is colour 3
    int j;
    for(i = 0; i < nv; i++)
        for(j = 0; j < num_available_colours[i]; j++) {
            int col_min_one = available_colours[i][j] - 1;
            ADDELEMENT1(&nautyg_gadget[i], nv + col_min_one);
            ADDELEMENT1(&nautyg_gadget[nv + col_min_one], i);
        }
    
}

/******************************************************************************/

//compare_array contains the tuple to which array should be compared
//If is_sim_pair = 1, best_pair[] contains:
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
//Where N(ai) \seteq N(bi)
static void
make_all_possible_combinations(unsigned char array[], int array_size, 
        unsigned char current_array[], int current_array_size, unsigned char compare_array[], 
        unsigned char best_pair[], int use_col_dom_graph) {
    if(is_sim_pair)
        return;
    
    if(current_array_size == array_size) { //Combination formed
        int i;
        if(!use_col_dom_graph) {
            for(i = 0; i < array_size; i++)
                if(!NBH_IS_SUBSET_OF(compare_array[i], current_array[i]) ||
                        !((available_colours_bitvector[compare_array[i]] & available_colours_bitvector[current_array[i]]) == available_colours_bitvector[current_array[i]]))
                    break;
            //else
            //N(i) \subseteq N(j)
            //And L(i) \superseteq L(j)
            if(i == array_size) {
                is_sim_pair = 1;
                for(i = 0; i < array_size; i++) {
                    best_pair[i] = compare_array[i];
                    best_pair[array_size + i] = current_array[i];
                }

                return;
            }

            for(i = 0; i < array_size; i++)
                if(!NBH_IS_SUBSET_OF(current_array[i], compare_array[i]) ||
                        !((available_colours_bitvector[compare_array[i]] & available_colours_bitvector[current_array[i]]) == available_colours_bitvector[compare_array[i]]))
                    break;            
            if(i == array_size) {
                is_sim_pair = 1;

                for(i = 0; i < array_size; i++) {
                    best_pair[array_size + i] = compare_array[i];
                    best_pair[i] = current_array[i];
                }

                return;
            }
        } else {
            fprintf(stderr, "Error: hidden edges not supported yet!\n");
            exit(1);
        }

    } else {
        int i;
        for(i = 0; i < array_size; i++)
            if(!ISMARKED(i)) {
                MARK(i);
                current_array[current_array_size] = array[i];
                make_all_possible_combinations(array, array_size, current_array,
                        current_array_size + 1, compare_array, best_pair, use_col_dom_graph);
                UNMARK(i);
            }
    }
    
}

/******************************************************************************/

//Tests if the pair is a similar pair.
//Important: the total size of possible_similar_pair is assumed to be 2*size_single
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
static int
is_similar_pair(unsigned char possible_similar_pair[], int size_single, int use_col_dom_graph) {
    RESETMARKS; //Don't forget!
    unsigned char array[size_single];
    unsigned char compare_array[size_single];
    unsigned char current_array[size_single];
    
    int i;
    for(i = 0; i < size_single; i++) {
        compare_array[i] = possible_similar_pair[i];
        array[i] = possible_similar_pair[i+size_single];
    }
    
    is_sim_pair = 0;
    make_all_possible_combinations(array, size_single, current_array, 0, compare_array, 
            possible_similar_pair, use_col_dom_graph);
    
    return is_sim_pair;
}

/******************************************************************************/

//Returns 1 if the graph contains a disjoint similar edgepair (see Sawada et al. Lemma 3.1 case m=2)
//else returns 0.
//If 1 is returned, best_similar_edgepair points to the *best* similar vertex which should be destroyed in the next step
//best_similar_edgepair[0] = a1
//best_similar_edgepair[1] = a2
//best_similar_edgepair[2] = b1
//best_similar_edgepair[3] = b2
static int contains_similar_edgepair(unsigned char best_similar_edgepair[4], int use_col_dom_graph) {
    //Search for all possible disjoint edgepairs
    //Min sum seems to be best (although it doesn't make that much difference...)
    unsigned char degree_current_best_small = 2*MAXN;
    unsigned char currently_best_similar_edgepair[4];
    
    int contains_sim_edgepair = 0;
    unsigned char i, j, k, l, next, next0;
    for(i = 0; i < nv - 3; i++) {
        for(j = 0; j < adj[i]; j++) {
            next = current_graph[i][j];
            if(i < next) {
                for(k = i + 1; k < nv - 1; k++) {
                    if(k != next) { // k != i is automatically implied because k >= i+1
                        for(l = 0; l < adj[k]; l++) {
                            next0 = current_graph[k][l];
                            if(k < next0 && next0 != next) {
                                //Edgepair i next k next0
                                //fprintf(stderr, "edgepair: (%d %d) (%d %d)\n", i, next, k, next0);
                                
                                //Remove vertices from edges from neighbourhoods
                                DELELEMENT1(&nautyg[i], next);
                                DELELEMENT1(&nautyg[next], i);
                                DELELEMENT1(&nautyg[k], next0);
                                DELELEMENT1(&nautyg[next0], k);
                                
                                best_similar_edgepair[0] = i;
                                best_similar_edgepair[1] = next;
                                best_similar_edgepair[2] = k;
                                best_similar_edgepair[3] = next0;                                
                                contains_sim_edgepair = is_similar_pair(best_similar_edgepair, 
                                        2, use_col_dom_graph);  
                                
                                //Restore neighbourhoods
                                ADDELEMENT1(&nautyg[i], next);
                                ADDELEMENT1(&nautyg[next], i);
                                ADDELEMENT1(&nautyg[k], next0);
                                ADDELEMENT1(&nautyg[next0], k);
                                
                                if(contains_sim_edgepair) {
                                    //int mindeg = MIN(adj[best_similar_edgepair[0]], adj[best_similar_edgepair[1]]);
                                    int mindeg = adj[best_similar_edgepair[0]] + adj[best_similar_edgepair[1]];
                                    if(mindeg < degree_current_best_small) {
                                        degree_current_best_small = mindeg;
                                        currently_best_similar_edgepair[0] = best_similar_edgepair[0];
                                        currently_best_similar_edgepair[1] = best_similar_edgepair[1];
                                        currently_best_similar_edgepair[2] = best_similar_edgepair[2];
                                        currently_best_similar_edgepair[3] = best_similar_edgepair[3];
                                    }
                                    //return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }    
    
    if(degree_current_best_small < MAXN) {
        //fprintf(stderr, "nv: %d, num sim edges: %d\n", nv, num_sim_edgepairs);
        best_similar_edgepair[0] = currently_best_similar_edgepair[0];
        best_similar_edgepair[1] = currently_best_similar_edgepair[1];
        best_similar_edgepair[2] = currently_best_similar_edgepair[2];
        best_similar_edgepair[3] = currently_best_similar_edgepair[3];          
        
        return 1;
    } else
        return 0;
    
}

/******************************************************************************/

/**
 * Searches for pairs of (nonadjacent) vertices in current_graph_mb which cannot have the same colour.
 * For these pairs an edge is added in nauty_graph_col_dom.
 * 
 * This is tested by adding a diamond-gadget.
 */
//Important: it is assumed that current_graph_mb is filled in and represents the graph 
//where certain vertices are removed (actually they are not removed but are isolated vertices)
//Important: vertices in removed_vertices are labelled starting from 0, not 1!
static void
search_for_hidden_edges(setword removed_vertices, graph *nauty_graph_col_dom) {
    int num_edges_added = 0;

    int i, j;
    for(i = 0; i < nv; i++)
        if(!ISELEMENT1(&removed_vertices, i))
            for(j = i + 1; j < nv; j++)
                if(!ISELEMENT1(&removed_vertices, j) && !ISELEMENT1(GRAPHROW1(nautyg, i, MAXM), j)) {
                    //For any pair of nonadjacent vertices
                    
                    int valid_colouring_with_same_colour = 0;
                    int col;
                    for(col = 1; col <= num_colours && !valid_colouring_with_same_colour; col++)
                        if(ISELEMENT1(&available_colours_bitvector[i], col)
                                && ISELEMENT1(&available_colours_bitvector[j], col)) {
                            //col is a possible colour for both vertices
                            
                            //Now test if there is a valid colouring with i and j both have colour col
                            
                            //Remark: colour of removed vertex doesn't really matter since this is an isolated vertex
                            //But for efficiency reasons best to give it the same colour
                            setword same_colour_vertices = removed_vertices;
                            ADDELEMENT1(&same_colour_vertices, i);
                            ADDELEMENT1(&same_colour_vertices, j);            

                            if(is_colourable_list_colouring_mb_same_colour_vertices(current_graph_mb,
                                    same_colour_vertices, col))
                                valid_colouring_with_same_colour = 1;
                            
                        }
                        
                    if(!valid_colouring_with_same_colour) {
                        //i.e. there is no valid colouring where i and j have the same colour
                        //so add a hidden edge between i and j
                        ADDELEMENT1(&nauty_graph_col_dom[i], j);
                        ADDELEMENT1(&nauty_graph_col_dom[j], i);
                        num_edges_added++;                        
                    }
                }
    //fprintf(stderr, "nv=%d, num edges added: %d\n", nv, num_edges_added);     
    
}

/******************************************************************************/

/**
 * Transforms current_graph into current_graph_mb where the vertices from
 * removed_vertices are isolated.
 */
//Important: vertices in removed_vertices are labelled starting from 0, not 1!
static void
make_graph_with_removed_vertices(setword removed_vertices) {
    //fprintf(stderr, "graph before:\n");
    //printgraph(current_graph, adj, nv);

    //First make minibaum format graph where removed vertices have degree 0
    current_graph_mb[0][0] = nv;
    int i, j;
    for(i = 1; i <= nv; i++) {
        adj_mb[i] = 0;
        if(!ISELEMENT1(&removed_vertices, i - 1)) {
            for(j = 0; j < adj[i - 1]; j++)
                if(!ISELEMENT1(&removed_vertices, current_graph[i - 1][j]))
                    current_graph_mb[i][adj_mb[i]++] = current_graph[i - 1][j] + 1;
        }

        //Is necessary as it is used in the colouring routines
        current_graph_mb[i][adj_mb[i]] = leer;
    }

    //fprintf(stderr, "graph after removing edge %d %d\n", k, current_graph[k][l]);
    //printgraph_minibaum(current_graph_mb, adj_mb, nv);    
}

/******************************************************************************/

//Compute the colour domination graph of a given removed vertex
static void
compute_colour_domination_graphs_vertex_removed_given(unsigned char removed_vertex) {
    setword removed_vertices = 0;
    ADDELEMENT1(&removed_vertices, removed_vertex);
    
    //fprintf(stderr, "removing vertex %d\n", removed_vertex);

    make_graph_with_removed_vertices(removed_vertices);

    copy_nauty_graph(current_graph, adj, nv, nautyg_colour_dominated_vertex_removed[removed_vertex]);
    search_for_hidden_edges(removed_vertices, nautyg_colour_dominated_vertex_removed[removed_vertex]);
}

/******************************************************************************/

/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - v.
 */
//Important: it is assumed that the col dom graph of vertices which are ISMARKED2
//were already determined!
static void
compute_colour_domination_graphs_vertex_removed() {
    int removed_vertex;
    for(removed_vertex = 0; removed_vertex < nv; removed_vertex++)
            compute_colour_domination_graphs_vertex_removed_given(removed_vertex);
    
}

/******************************************************************************/

//Returns 1 if the new vertex n can be coloured with the same colour as avoided_vertex,
//else returns 0
//If it cannot be coloured with the same colour (i.e. 0 is returned), the similar vertex
//wasn't destroyed by the expansion
//If isolated_vertices == 0, 1 is also returned
static int
similar_element_is_destroyed(setword isolated_vertices, unsigned char avoided_vertex) {
    //Verify that similar element is indeed now destroyed
    if(isolated_vertices != 0) {
        //Remark: doesn't have to do this each time (could store into local variable)
        //but this is not a bottleneck
        make_graph_with_removed_vertices(isolated_vertices);
        
        ADDELEMENT1(&isolated_vertices, avoided_vertex);
        ADDELEMENT1(&isolated_vertices, nv - 1);        

        int col;
        for(col = 1; col <= num_colours; col++)
            if(ISELEMENT1(&available_colours_bitvector[avoided_vertex], col)
                    && ISELEMENT1(&available_colours_bitvector[nv - 1], col)) {
                //col is a possible colour for both vertices

                //Now test if there is a valid colouring where avoided_vertex and  nv - 1 both have colour col

                //Remark: colour of removed vertex doesn't really matter since this is an isolated vertex
                //But for efficiency reasons best to give it the same colour
                setword same_colour_vertices = isolated_vertices;
                ADDELEMENT1(&same_colour_vertices, avoided_vertex);
                ADDELEMENT1(&same_colour_vertices, nv - 1);

                if(is_colourable_list_colouring_mb_same_colour_vertices(current_graph_mb,
                        same_colour_vertices, col))
                    return 1; //I.e. no hidden edge between avoided_vertex and nv - 1

            }        
        
        //i.e. there is a hidden edge between avoided_vertex and nv - 1
        return 0;
        
    } else
        return 1;
}

/******************************************************************************/

//For now just testing if adj[i] < num_available_colours[i]
//If 1 is returned, best_small_vertex points to the *best* small vertex
//It doesn't matter if the first one or the smallest or largest one is taken
//Girth case: seems best always to take small vertex with largest label!
static int contains_too_small_vertex(unsigned char *best_small_vertex) {
    int i;
    int current_best = -1;
    for(i = 0; i < nv; i++)
        if(adj[i] < num_available_colours[i]) {
            current_best = i;
        }
    //Helps a little bit if returning a vertex which is part of a triangle (in the diamondfree case)
    if(current_best != -1) {
        *best_small_vertex = current_best;
        return 1;
    } else {
        //Test if there is a vertex which has an available colour which is not contained
        //in the available colours of its neighbourhood
        
        //With this the diamondfree case no longer terminates...
        //So maybe better just give this second part a lower priority?
/*
        int j;
        for(i = 0; i < nv; i++) {
            setword available_colours_neighbourhood_union = 0;
            for(j = 0; j < adj[i]; j++)
                available_colours_neighbourhood_union |= available_colours_bitvector[current_graph[i][j]];

            if((available_colours_bitvector[i] & available_colours_neighbourhood_union) != available_colours_bitvector[i]) {
                //Contains a colour which is not in the union of neighbourhood colours
                current_best = i;
                //return 1;
            }
        }
*/
        if(current_best != -1) {
            *best_small_vertex = current_best;
            return 1;
        } else
            return 0;
    }
}

/******************************************************************************/

/**
 * Main method of the generation algorithm.
 * Takes the current graph and performs all possible (canonical) extensions on it.
 * It is assumed that the current graph is canonical.
 */
//If isolated_vertices != 0, avoided_vertex contains the vertex which was avoided by the extension
//isolated_vertices are the vertices which are to be "removed" in similar_element_is_destroyed()
//to test if the similar vertex / edge / triangle / etc. is indeed destroyed by the expansion
//If max_colours_nbh_colouring > 0, it means the last expansion destroyed a small vertex
//If cutvertices > 1, it is tested that the number of components when these vertices
//are removed is at least 1 less than num_components_parent

//Important: it is assumed that the graph is (Pt,Cu)-free
static void
extend_Pfree(setword isolated_vertices, unsigned char avoided_vertex) {
    //Can now happen since starting from multiple startgraphs
    if(nv > maxnv) {
        fprintf(stderr, "Error: nv is too big: %d vs max: %d\n", nv, maxnv);
        exit(1);
    }    
    
    //Warning: it is assumed that it has already been verified that the graph is (Pt,Cu)-free!

    if(!similar_element_is_destroyed(isolated_vertices, avoided_vertex)) {
        times_similar_element_not_destroyed[nv]++;
        //previous_extension_was_not_colourable = 1;
        //Larger extensions also certainly won't destroy the similar element
        //since there will also be a hidden edge between nv-1 and the avoided_vertex
        return; //prune
    } else
        times_similar_element_destroyed[nv]++;    
    
    
    //Test this before calling nauty & splay
    //TODO: doesn't help much since most graphs at this point are colourable?
    //int is_col = is_colourable();
    //int is_col = is_colourable_list_colouring();
    //int is_col = 1;
    int is_col = is_colourable_list_colouring();
    if(!is_col) {
        times_graph_not_colourable[nv]++;
        
        //No, can't do this anymore
        //Very important: when using this opt, the col test must be performed
        //AFTER the P-free test!!! Since if this graph is not colourable 
        //but contains an induced Px, one of the other extensions might still yield a P-free critical graph!
        //if(!output_vertex_critical_graphs)
        //    previous_extension_was_not_colourable = 1;
        
        //Now test if the graph is vertex-critical
        //We don't need to remove the last vertex since we know its parent is k-colourable
        
        //Is significantly faster to do this before iso test!
        //Prune if not vertex critical
        if(!is_list_vertex_critical()) {
            times_graph_not_vertex_critical[nv]++;
            
            return; //Prune, so we also don't need to call nauty
        } else
            times_graph_vertex_critical[nv]++;

    } else {
        times_graph_colourable[nv]++;
    }
    
    make_nauty_gadget_graph();
    
    //malloc is not faster, but it uses less memory than using MAXN!
    graph *can_form_gadget = (graph *) malloc(sizeof (graph) * (nv + 3));
    if(can_form_gadget == NULL) {
        fprintf(stderr, "Error: can't get more space\n");
        exit(1);
    }
    //Don't forget +3 for triangle gadget!
    nauty(nautyg_gadget, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM, nv + 3, can_form_gadget);
    //nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM, nv, can_form);
    //memcpy(can_form, nautyg_canon, sizeof (graph) * nv);
    
    int is_new_node;
    splay_insert(&list_of_worklists[nv], can_form_gadget, &is_new_node);
    if(!is_new_node) {
        num_graphs_generated_iso[nv]++;
        free(can_form_gadget); //Don't forget to free!
        return;
    } else //i.e. is canon
        num_graphs_generated_noniso[nv]++;
    
    if(!is_col) {
        //We know that the graph is in the class and is possibly vertex-critical
        //So output graph
        
#ifdef TEST_IF_CRITICAL        
        //Best to only test this here since the is_edge_critical() test is quite expensive
        if(!output_vertex_critical_graphs && !is_edge_critical())
            return;
#endif        
        
        output_graph(current_graph, adj, nv);
        
/*
        fprintf(stderr, "Critical graph found\n");
        printgraph(current_graph, adj, nv);
        int i, j;
        for(i = 0; i < nv; i++) {
            fprintf(stderr, "av colours %d: ", i);
            for(j = 0; j < num_available_colours[i]; j++) 
                fprintf(stderr, "%d ", available_colours[i][j]);
            fprintf(stderr, "\n");
        }
        
        fprintf(stderr, "----\n");
*/
        
        
        return; //prune since if current_graph not k-col, its children also wont be k-colourable
    } 
    
    if(nv == splitlevel) {
        splitcounter++;
        
        if(splitcounter == modulo)
            splitcounter = 0;
        
        if(splitcounter != rest)
            return;
    }    
    
    if(nv == maxnv) {
        //output_graph(current_graph, adj, nv);

        return;
    }
    
    //Now apply all possible expansions
    
    //Important: compute possible expansions on the fly! (Is faster + segfault if array becomes too big)
    unsigned char current_set[MAXN];   
    
    unsigned char best_small_similar_vertex;
    unsigned char other_larger_similar_vertex;
    
    //printgraph(current_graph, adj, nv);
    //printgraph_nauty(nautyg, nv);

    if(contains_similar_vertices(&best_small_similar_vertex, &other_larger_similar_vertex)) {
        times_contains_similar_vertices[nv]++;
        
        //Apply all possible expansions which destroy the similar vertex, i.e.:
        //New vertex is connected to best_small_similar_vertex, but not to other_larger_similar_vertex
        
        current_set[0] = best_small_similar_vertex;
        setword forbidden_vertices = 0;
        ADDELEMENT1(&forbidden_vertices, best_small_similar_vertex);
        ADDELEMENT1(&forbidden_vertices, other_larger_similar_vertex);

        setword current_set_bitvector = 0;
        ADDELEMENT1(&current_set_bitvector, best_small_similar_vertex);

        setword isolated_vertices = 0;
        ADDELEMENT1(&isolated_vertices, best_small_similar_vertex);        
        
        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1,
                current_set_bitvector, 0, forbidden_vertices, isolated_vertices, other_larger_similar_vertex);
        
    } else {
        times_doesnt_contain_similar_vertices[nv]++;

        compute_colour_domination_graphs_vertex_removed();
        if(contains_similar_vertices_col_dom(&best_small_similar_vertex, &other_larger_similar_vertex)) {
            times_contains_similar_vertices_col_dom[nv]++;

            //Apply all possible expansions which destroy the similar vertex, i.e.:
            //New vertex is connected to best_small_similar_vertex, but not to other_larger_similar_vertex

            current_set[0] = best_small_similar_vertex;
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, best_small_similar_vertex);
            ADDELEMENT1(&forbidden_vertices, other_larger_similar_vertex);

            setword current_set_bitvector = 0;
            ADDELEMENT1(&current_set_bitvector, best_small_similar_vertex);

            setword isolated_vertices = 0;
            ADDELEMENT1(&isolated_vertices, best_small_similar_vertex);        

            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1,
                    current_set_bitvector, 0, forbidden_vertices, isolated_vertices, other_larger_similar_vertex);

        } else {
            times_doesnt_contain_similar_vertices_col_dom[nv]++;

            unsigned char best_small_vertex = MAXN;
            if(contains_too_small_vertex(&best_small_vertex)) {
                times_contains_small_vertex[nv]++;
                
                current_set[0] = best_small_vertex;
                setword forbidden_vertices = 0;
                ADDELEMENT1(&forbidden_vertices, best_small_vertex);

                setword current_set_bitvector = 0;
                ADDELEMENT1(&current_set_bitvector, best_small_vertex);                

                setword isolated_vertices = 0;
                construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1,
                        current_set_bitvector, 0, forbidden_vertices, isolated_vertices, 0);

            } else {
                times_doesnt_contain_small_vertex[nv]++;

                unsigned char similar_edgepair[4];

                if(contains_similar_edgepair(similar_edgepair, 0)) {
                    times_contains_similar_edgepair[nv]++;

                    setword isolated_vertices = 0;
                    ADDELEMENT1(&isolated_vertices, similar_edgepair[0]);
                    ADDELEMENT1(&isolated_vertices, similar_edgepair[1]);

                    //Generate sets which are adjacent to similar_edgepair[0] but not to similar_edgepair[2]
                    current_set[0] = similar_edgepair[0];
                    setword current_set_bitvector = 0;
                    ADDELEMENT1(&current_set_bitvector, current_set[0]);
                    setword forbidden_vertices = 0;
                    ADDELEMENT1(&forbidden_vertices, similar_edgepair[0]);
                    ADDELEMENT1(&forbidden_vertices, similar_edgepair[2]);
                    construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1,
                            current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_edgepair[2]);

                    //Generate sets which are adjacent to similar_edgepair[1] but not to similar_edgepair[3]
                    current_set[0] = similar_edgepair[1];
                    current_set_bitvector = 0;
                    ADDELEMENT1(&current_set_bitvector, current_set[0]);
                    forbidden_vertices = 0;
                    ADDELEMENT1(&forbidden_vertices, similar_edgepair[1]);
                    ADDELEMENT1(&forbidden_vertices, similar_edgepair[3]);
                    construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1,
                            current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_edgepair[3]);

                    //TODO: duplicated sets will be generated in case they contain both 
                    //similar_edgepair[0] and similar_edgepair[1]
                    //But probably this won't matter that much?

                    //VERY IMPORTANT:
                    //Must perform both expansions like this (each time with a different avoided vertex)
                    //because similar_element_is_destroyed() relies that both will be performed
                    //or else not all graphs will be generated!            


                } else {
                    times_doesnt_contain_similar_edgepair[nv]++;

                    times_no_lemmas_applicable[nv]++;

                    //Just use the same method with no forbidden vertices
                    //Total sets: 2^n-1
                    setword forbidden_vertices = 0;
                    construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                            0, 0, 0, forbidden_vertices, 0, 0);
                }
            }
        }
    }
    
}

/******************************************************************************/

//2^num_colours - 1 combinations
static void select_colour_subset_recursion(setword isolated_vertices, unsigned char avoided_vertex,
        int current_colour) {
    
    //Only generating graphs where each vertex has at least 2 possible colours
    //if(num_available_colours[nv - 1] > 0) {
    if(num_available_colours[nv - 1] >= MIN_COLOUR_SET_SIZE) {
    //if(num_available_colours[nv - 1] > 1) {
    //if(num_available_colours[nv - 1] > 0 && num_available_colours[nv - 1] <= 2) {
    //if(num_available_colours[nv - 1] > 1 && num_available_colours[nv - 1] <= 2) {
/*
        int i;
        fprintf(stderr, "combination: ");
        for(i = 0; i < num_available_colours[nv - 1]; i++)
            fprintf(stderr, "%d ", available_colours[nv - 1][i]);
        fprintf(stderr, "\n");
*/
        
        //Extend
        extend_Pfree(isolated_vertices, avoided_vertex);
    }
    
    int i;
    for(i = current_colour; i <= num_colours; i++) {
        available_colours[nv - 1][num_available_colours[nv - 1]] = i;
        num_available_colours[nv - 1]++;
        ADDELEMENT1(&available_colours_bitvector[nv - 1], i);
        
        //Recursion
        select_colour_subset_recursion(isolated_vertices, avoided_vertex, i + 1);
        
        num_available_colours[nv - 1]--;
        DELELEMENT1(&available_colours_bitvector[nv - 1], i);
    }
}

/******************************************************************************/

//Important: it is assumed that the graph is (Pt,Cu)-free
static void choose_colours_for_last_vertex_in_all_ways(setword isolated_vertices, unsigned char avoided_vertex) {
    
    num_available_colours[nv - 1] = 0;    
    available_colours_bitvector[nv - 1] = 0;
    
    //Stel available_colours[nv - 1] in
    
    select_colour_subset_recursion(isolated_vertices, avoided_vertex, 1);
    
}

/******************************************************************************/

//Returns 1 if the graph contains a diamond which also contains the last vertex, else returns 0
//The last vertex must be in the diamond, since the parents were diamond-free
//int n: nv of the graph
static int
contains_induced_diamond() {
    if(adj[nv-1] < 2) //Won't happen very often
        return 0;
    
    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        int popcnt = POPCOUNT(intersect_neigh_n_b1);
        if(popcnt >= 2) {
            //Now test if the diamond is induced
            int a1 = -1;
            while((a1 = nextelement1(&intersect_neigh_n_b1, 1, a1)) >= 0) {
                int a2 = a1;
                while((a2 = nextelement1(&intersect_neigh_n_b1, 1, a2)) >= 0) {
                    if(!ISELEMENT1(&nautyg[a1], a2))
                        return 1; //diamond is induced
                }
            }
        }
        if(popcnt >= 1) {
            //Find third vertex of the triangle
            int neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, -1);
            setword intersect_neigh_b1_b2 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *GRAPHROW1(nautyg, neighbour_b2, MAXM);
            if(POPCOUNT(intersect_neigh_b1_b2) >= 2) {
                //contains diamond; n is a degree 2 vertex of the diamond            
                //Now test if the diamond is induced
                int a = -1;
                while((a = nextelement1(&intersect_neigh_b1_b2, 1, a)) >= 0) {
                    if(a != nv - 1 && !ISELEMENT1(&nautyg[nv - 1], a))
                        return 1; //diamond is induced
                }                
            }
                
        }
    }
    
    return 0;
}


/**************************************************************************/

//Returns 1 if the graph is 2-colourable (i.e. bipartite), else returns 0
static int is_twocolourable() {
    int head, tail, vertex, need;

    int queue[nv];
    int colour[nv];

    int i;
    for(i = 0; i < nv; i++)
        colour[i] = -1;

    //Colour vertex 0 with colour 0
    queue[0] = 0;
    colour[0] = 0;

    head = 0;
    tail = 1;
    while(head < tail) {
        vertex = queue[head++];
        need = 1 - colour[vertex];
        for(i = 0; i < adj[vertex]; i++) {
            int neighbour = current_graph[vertex][i];
            if(colour[neighbour] < 0) {
                colour[neighbour] = need;
                queue[tail++] = neighbour;
            } else if(colour[neighbour] != need)
                return 0;
        }
    }
    
    for(i = 0; i < nv; i++)
        if(colour[i] == -1) {
            fprintf(stderr, "Error: not every vertex was coloured!\n");
            //Will happen if graph is disconnected
            exit(1);
        }
            
    
    return 1;
}

/******************************************************************************/

static void
test_if_graph_is_P_free_and_extend(setword isolated_vertices, unsigned char avoided_vertex) {

    if(nv > maxnv) {
        fprintf(stderr, "Error: nv is too big: %d vs max: %d\n", nv, maxnv);
        exit(1);
    }    
    
    num_graphs_generated[nv]++;

    //It is a lot faster to test if the graph contains a K4 before calling nauty!
    
    //The fastest option seems to be: K4, C5, W5, nauty, P-free
    //But K4, W5, C5, nauty, P-free is hardly any slower
    //Remark: may depend on specific parameters, so test for larger cases!

    if(test_if_bipartite && !is_twocolourable()) {
        
        return; //prune
    }
    
    if(test_if_diamondfree && contains_induced_diamond()) {

        return; //prune
    }    
    
    if(test_if_contains_forbidden_cycle) {
        //Since the parents were C-free, the last vertex must be in the forbidden cycle
        //It is a lot faster to test this before calling nauty!
        if(contains_forbidden_induced_cycle(forbidden_cycle_size)) {
            counts_contains_forbidden_induced_cycle[nv]++;
            return; //prune
        } else
            counts_doesnt_contain_forbidden_induced_cycle[nv]++;
    }
    

/*
    //Actually performing induced C5 test in contains_W5(), so if the current graph
    //doesn't contain an induced C5, it also won't contain a W5, so no need to test that (certainly helps)!
    if(num_colours == 3 && !test_if_trianglefree &&
            !(test_if_contains_forbidden_cycle && forbidden_cycle_size == 5)) {
        //Test if contains W5
        //It doesn't really matter much if this is called before or after nauty test...
        if(nv > 6 && contains_W5(nv)) { //The last vertex must be in the W5, since the parents were W5-free
            counts_contains_W5[nv]++;
            previous_extension_was_not_colourable = 1;
            return; //prune
        } else
            counts_doesnt_contain_W5[nv]++;
    }
*/

/*
    //In the triangle-free case it is faster to test this before the P-free test
    //Especially for smaller paths (e.g. P6-free, but also a bit for P7-free)
    int test_similar_element_destroyed_first = !test_if_trianglefree;
    if(test_similar_element_destroyed_first) {
        if(!similar_element_is_destroyed(isolated_vertices, avoided_vertex)) {
            times_similar_element_not_destroyed[nv]++;
            previous_extension_was_not_colourable = 1;
            //Larger extensions also certainly won't destroy the similar element
            //since there will also be a hidden edge between nv-1 and the avoided_vertex
            return; //prune
        } else
            times_similar_element_destroyed[nv]++;
    }
*/

    //P-free test is much more expensive than col test, but nearly all graphs are colourable for most cases
    //so better to do P-free test first
    
    //It is faster to do P-free test before isomorphism test
    //(certainly for large graphs, otherwise too much graphs in mem and most are not isomorphic)
    if(contains_forbidden_induced_path()) {
        times_graph_not_Pfree[nv]++;
        return; //prune since if current_graph contains a forbidden induced path, so will it's children
    } else
        times_graph_Pfree[nv]++;          
    
    
/*
    //Seems to be slightly faster to perform this before is_col test (but after the other cheaper tests) 
    //Very important: don't run this test after noniso test, since that may lead
    //to missed graphs if POPCOUNT(isolated_vertices) > 1!!!
    //i.e. one possibility not destroyed (but other is). Other option will be generated
    //in the next step, but if this test is performed after noniso test this graph won't be accepted!
    if(!test_similar_element_destroyed_first) { //For P6-free graphs, it is faster to test this before P-free test
        if(!similar_element_is_destroyed(isolated_vertices, avoided_vertex)) {
            times_similar_element_not_destroyed[nv]++;
            previous_extension_was_not_colourable = 1;
            //Larger extensions also certainly won't destroy the similar element
            //since there will also be a hidden edge between nv-1 and the avoided_vertex
            return; //prune
        } else
            times_similar_element_destroyed[nv]++;
    }    
*/
    
    
    //Ok, only now assign colours in all possible ways
    //So we don't have to test (Pt,Cu)-freeness over and over again!
    //2^num_colours - 1 combinations
    choose_colours_for_last_vertex_in_all_ways(isolated_vertices, avoided_vertex);
    
}

/******************************************************************************/

static void
init_construction() {
    //options.userautomproc = save_generators;
    options.defaultptn = TRUE;
    //options.getcanon = FALSE;
    options.getcanon = TRUE;
    
    //int (*previous_induced_path)[MAXN + 1][NUM_PREV_SETS][MAXN];
    previous_induced_paths = malloc(sizeof (int) * (MAXN + 1) * NUM_PREV_SETS * MAXN);
    if(previous_induced_paths == NULL) {
        fprintf(stderr, "Error: Can't get enough memory while creating nautyg_colour_dominated_edge_removed\n");
        exit(1);
    }

    //Important: make sure that everything is initialised to zero!
    int i, j, k;
    for(i = 0; i <= maxnv; i++)
        for(j = 0; j < NUM_PREV_SETS; j++)
            for(k = 0; k < MAXN; k++)
                (*previous_induced_paths)[i][j][k] = 0;
    
    //Initialise construction from the isolated vertex
    nv = 1;
    adj[0] = 0;

    copy_nauty_graph(current_graph, adj, nv, nautyg);

    //printgraph(current_graph, adj, nv);
    //printgraph_nauty(nautyg, nv);
    
    //For first vertex only 3 possibilities (but can test later just to check that isomorphism found)
    //1; 1,2; 1,2,3
    
    setword isolated_vertices = 0;
    test_if_graph_is_P_free_and_extend(isolated_vertices, 0);

}

/******************************************************************************/

static void
initialize_splitting() {
    if(modulo == 1)
        splitlevel = 0;
    else {
        splitlevel = MAX(maxnv - 5, MIN_SPLITLEVEL);
        splitlevel = MIN(splitlevel, MAX_SPLITLEVEL);
        splitlevel = MIN(splitlevel, maxnv - 1);
        fprintf(stderr, "Splitlevel is %d\n", splitlevel);
    }
}

/******************************************************************************/

static void
perform_wordsize_checks() {
    /* Checks to test if the WORDSIZE and sizeof setwords is valid */
    if(WORDSIZE != 32 && WORDSIZE != 64) {
        fprintf(stderr, "Error: invalid value for wordsize: %d\n", WORDSIZE);
        fprintf(stderr, "Valid values are 32 and 64\n");
        exit(1);
    }

    if(MAXN != WORDSIZE) {
        fprintf(stderr, "Error: MAXN is not equal to WORDSIZE: %d vs %d\n", MAXN, WORDSIZE);
        fprintf(stderr, "Please compile with option -DMAXN=WORDSIZE\n");
        exit(1);
    }
    
    if(MAXM != 1) {
        fprintf(stderr, "Error: expected MAXM 1 instead of %d\n", MAXM);
        exit(1);
    }

    if(WORDSIZE == 32) {
        fprintf(stderr, "32 bit mode\n");
        if(sizeof (unsigned int) < 4) {
            fprintf(stderr, "Error: unsigned ints should be at least 32 bit -- sorry.\n");
            exit(1);
        }
        if(sizeof (setword) < 4) {
            fprintf(stderr, "Error: this version relies on 32 bit setwords -- sorry.\n");
            exit(1);
        }
        if(sizeof (setword) != 4) {
            fprintf(stderr, "Error: expected 32 bit setwords\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "64 bit mode\n");
        if(sizeof (setword) != 8) {
            fprintf(stderr, "Error: this version relies on 64 bit setwords -- sorry.\n");
            exit(1);
        }
        if(sizeof (unsigned long long int) != 8) {
            fprintf(stderr, "Error: this version relies on 64 bit unsigned long long ints -- sorry.\n");
            exit(1);
        }
    }
}

/******************************************************************************/

static void
print_help(char * argv0) {
    fprintf(stderr, "Critical_Pfree_graphs version %s\n", VERSION);
    fprintf(stderr, "Usage: %s <number_of_vertices> c<x> P<x> [options]\n", argv0);
    fprintf(stderr, "At least the number of vertices, c<x> and P<x> must be specified.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Valid options are:\n");
    fprintf(stderr, "  c<x>: Searches for <x>-critical graphs (i.e. graphs which are NOT <x>-colourable).\n");
    fprintf(stderr, "  P<x>: Searches for P<x>-free graphs (i.e. graphs which do not contain P<x> as induced subgraph).\n");
    fprintf(stderr, "  C<x>: Searches for C<x>-free graphs (i.e. graphs which do not contain C<x> as induced subgraph).\n");
    fprintf(stderr, "  diamondfree: Searches for diamond-free graphs.\n");
    fprintf(stderr, "  bipartite: Searches for bipartite graphs.\n");
    fprintf(stderr, "  g<x>: Searches for graphs with girth at least <x>.\n");
    fprintf(stderr, "  vertexcritical: Output vertex-critical instead of critical graphs.\n");
    fprintf(stderr, "  mod <rest> <modulo>: Splits the generation in <modulo> (more or less equally big) parts. Here part <rest> will be executed.\n");    
}

/******************************************************************************/

/*
 * 
 */
int main(int argc, char** argv) {
    
    perform_wordsize_checks();
    nauty_check(WORDSIZE, MAXM, MAXN, NAUTYVERSIONID);

    if(argc == 2 && argv[1][0] == '-' && argv[1][1] == 'h') {
        print_help(argv[0]);
        exit(1);
    }

    if(argc < 3) {
        if(argc == 2 && argv[1][0] == '-' && argv[1][1] == 'h') {
            print_help(argv[0]);
        } else {
            fprintf(stderr, "Error: invalid number of arguments. At least the number of vertices, c<x> and P<x> must be specified.\n");
            fprintf(stderr, "Usage: %s <number_of_vertices> c<x> P<x> [options]\n", argv[0]);
            fprintf(stderr, "Execute '%s -h' for more extensive help.\n", argv[0]);
        }
        exit(1);
    } else {
        if(sscanf(argv[1], "%d", &maxnv)) {
            //if(maxnv > MAXN) {
            //+3 since adds triangle gadget!
            if(maxnv + 3 > MAXN) {
            //if(maxnv >= MAXN - 2) { 
                //Because of colour domination graph: here 2 new vertices are added
                //Here minibaum repres is also used (so +1), but this is never done on last level
                //No, now also done on last level
                
                //MAXN vertices forbidden since also working with minibaum current_graph representation!
                //However it wouldn't be too much work to modify the program to work with MAXN vertices as well
                //By: typedef unsigned char GRAPH[MAXN+1][MAXN];
                if(MAXN == 32) {
                    fprintf(stderr, "Error: maximum number of vertices is 32 in this version.\n");
                    fprintf(stderr, "Compile with option -DWORDSIZE=64 in order to be able to generate graphs with a higher order.\n");
                    fprintf(stderr, "Or alternatively use the command 'make 64bit'\n");
                    exit(1);
                } else {
                    fprintf(stderr, "Error: number of vertices is too big (limit is %d).\n", MAXN);
                    exit(1);
                }
            }
            if(MAXN == 64 && maxnv <= 32) {
                fprintf(stderr, "Info: it is recommended to use the 32 bit version for generating graphs with order <= 32\n");
            }

        } else {
            fprintf(stderr, "Error: number of vertices should be an integer.\n");
            exit(EXIT_FAILURE);
        }

        int i;
        for(i = 2; i < argc; i++) {
            switch(argv[i][0]) {
                case 'P':
                {
                    //If Px forbidden, max nv in path is x-1
                    max_path_size = atoi(argv[i] + 1) - 1;
                    if(max_path_size < 1) {
                        fprintf(stderr, "Error: invalid filter value: %d\n", max_path_size);
                        exit(1);
                    }
                    fprintf(stderr, "Info: only outputting graphs with no induced P%d's\n", max_path_size + 1);
                    break;
                }
                case 'C':
                {
                    //Only accepts graphs which are NOT k-colourable (on last level)
                    forbidden_cycle_size = atoi(argv[i] + 1);
                    if(forbidden_cycle_size < 1) {
                        fprintf(stderr, "Error: invalid filter value: %d\n", forbidden_cycle_size);
                        exit(1);
                    }
                    test_if_contains_forbidden_cycle = 1;
                    fprintf(stderr, "Info: only outputting graphs with no induced C%d's\n", forbidden_cycle_size);
                    break;
                }                
                case 'c':
                {
                    //Only accepts graphs which are NOT k-colourable (on last level)
                    num_colours = atoi(argv[i] + 1);
                    if(num_colours < 1) {
                        fprintf(stderr, "Error: invalid filter value: %d\n", num_colours);
                        exit(1);
                    }
                    fprintf(stderr, "Info: only outputting graphs which are NOT %d-colourable\n", num_colours);
                    break;
                }
                case 'm':
                {
                    if(strcmp(argv[i], "mod") == 0) {
                        if(i + 2 < argc) {
                            i++;
                            rest = atoi(argv[i]);
                            i++;
                            modulo = atoi(argv[i]);
                            if(rest >= modulo) {
                                fprintf(stderr, "Error: rest (%d) must be smaller than modulo (%d).\n", rest, modulo);
                                exit(1);
                            }
                        } else {
                            fprintf(stderr, "Error: option 'mod': missing rest or modulo\n");
                            exit(1);
                        }
                        fprintf(stderr, "Warning: splitting doesn't work very well with this program (a lot of isomorphic copies). Only works well with large mod.\n");
                    } else {
                        fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                        fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                        exit(1);
                    }
                    break;
                }    
                case 'd':
                {
                    if(strcmp(argv[i], "diamondfree") == 0) {
                        test_if_diamondfree = 1;
                        fprintf(stderr, "Info: only generating graphs which are diamond-free\n");
                        fprintf(stderr, "Warning: for diamond-free case small vertex needs higher priority"
                                " than similar edges in order to allow termination!\n");
                    }
                    break;
                }  
                case 'b':
                {
                    if(strcmp(argv[i], "bipartite") == 0) {
                        test_if_bipartite = 1;
                        fprintf(stderr, "Info: only generating graphs which are bipartite\n");
                        fprintf(stderr, "Info: also turning C3-free test on (is faster)\n");
                        test_if_trianglefree = 1;
                    }
                    break;
                }                  
                case 'v':
                {
                    if(strcmp(argv[i], "vertexcritical") == 0) {
                        output_vertex_critical_graphs = 1;
                        fprintf(stderr, "Info: outputting vertex critical graphs\n");
                    }
                    break;
                }                 
                default:
                {
                    fprintf(stderr, "Error: invalid option: %s\n", argv[i]);
                    fprintf(stderr, "Execute '%s -h' for a list of possible options.\n", argv[0]);
                    exit(1);
                }
            }
        }
    }
    
    if(num_colours != 3) {
        fprintf(stderr, "Error: only 3-colourability supported for now!\n");
        exit(1);
    }
    
    if(test_if_contains_forbidden_cycle)
        fprintf(stderr, "Program for outputting %d-list-critical (P%d,C%d)-free graphs\n", num_colours + 1, max_path_size + 1, forbidden_cycle_size);
    else
        fprintf(stderr, "Program for outputting %d-list-critical P%d-free graphs\n", num_colours + 1, max_path_size + 1);
    
    fprintf(stderr, "Info: NUM_PREV_SETS: %d\n", NUM_PREV_SETS);
    
    
    if(output_vertex_critical_graphs)
        fprintf(stderr, "Info: only outputting k-vertex-critical graphs\n");
    else {
#ifdef TEST_IF_CRITICAL
        fprintf(stderr, "Info: only outputting k-critical graphs\n");
        //fprintf(stderr, "Error: only vertex-criticality supported for now (else might be very slow)\n");
        //exit(1);
#else
        fprintf(stderr, "Info: outputting possibly k-critical graphs\n");
#endif  
    }
    
    if(test_if_contains_forbidden_cycle && forbidden_cycle_size == 3) {
        test_if_trianglefree = 1;
        test_if_contains_forbidden_cycle = 0;
        if(test_if_diamondfree) {
            test_if_diamondfree = 0;
            fprintf(stderr, "Info: diamond-free test is obsolete since graphs are triangle-free\n");
        }        
    }    

    fprintf(stderr, "Info: min colour set size: %d\n", MIN_COLOUR_SET_SIZE);
    fprintf(stderr, "Info: in criticality test not testing if more colours can be added to the sets of colours without making the graph colourable\n");
    
    //For the timing
    struct tms TMS;

    int i;
    for(i = 0; i < argc; i++)
        fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");

    //Start construction
    initialize_splitting();

    init_construction();
    
    free(previous_induced_paths);

    times(&TMS);
    unsigned int savetime = (unsigned int) TMS.tms_utime;
    
/*
    for(i = 5; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim vertices: %llu (%.2f%%)\n", i, times_contains_similar_vertices[i],
            (double) times_contains_similar_vertices[i] / (times_contains_similar_vertices[i] + times_doesnt_contain_similar_vertices[i]) * 100);    
    
    for(i = 5; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim vertices col_dom: %llu (%.2f%%)\n", i, times_contains_similar_vertices_col_dom[i],
            (double) times_contains_similar_vertices_col_dom[i] / (times_contains_similar_vertices_col_dom[i] + times_doesnt_contain_similar_vertices_col_dom[i]) * 100);

    for(i = 5; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains small vertex: %llu (%.2f%%)\n", i, times_contains_small_vertex[i],
            (double) times_contains_small_vertex[i] / (times_contains_small_vertex[i] + times_doesnt_contain_small_vertex[i]) * 100);    
    
    for(i = 5; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim edgepair: %llu (%.2f%%)\n", i, times_contains_similar_edgepair[i],
            (double) times_contains_similar_edgepair[i] / (times_contains_similar_edgepair[i] + times_doesnt_contain_similar_edgepair[i]) * 100);    
    
    for(i = 5; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times no lemmas applicable: %llu\n", i, times_no_lemmas_applicable[i]);    
*/
        
    
/*
    if(test_if_contains_forbidden_cycle)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times C%d-free: %llu (%.2f%%)\n", i, forbidden_cycle_size, counts_doesnt_contain_forbidden_induced_cycle[i],
                    (double) counts_doesnt_contain_forbidden_induced_cycle[i] / (counts_doesnt_contain_forbidden_induced_cycle[i] + counts_contains_forbidden_induced_cycle[i]) * 100);    
    
    for(i = 8; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times P%d found previous: %llu (%.2f%%)\n", i, max_path_size + 1, times_induced_path_found_previous[i],
                (double) times_induced_path_found_previous[i] / (times_induced_path_found_previous[i] + times_induced_path_not_found_previous[i]) * 100);                            
    
    for(i = 8; i <= maxnv; i++)
       fprintf(stderr, "Nv=%d, times P-free: %llu (%.2f%%)\n", i, times_graph_Pfree[i],
                (double) times_graph_Pfree[i] / (times_graph_Pfree[i] + times_graph_not_Pfree[i]) * 100);   
    
    for(i = 8; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times colourable: %llu (%.2f%%)\n", i, times_graph_colourable[i],
                (double) times_graph_colourable[i] / (times_graph_colourable[i] + times_graph_not_colourable[i]) * 100);            
    
    for(i = 1; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times vertex-critical: %llu (%.2f%%)\n", i, times_graph_vertex_critical[i],
                (double) times_graph_vertex_critical[i] / (times_graph_vertex_critical[i] + times_graph_not_vertex_critical[i]) * 100);            
*/
    
/*
    for(i = 6; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times similar element NOT destroyed: %llu (%.2f%%)\n", i, times_similar_element_not_destroyed[i],
                (double) times_similar_element_not_destroyed[i] / (times_similar_element_not_destroyed[i] + times_similar_element_destroyed[i]) * 100);                
*/
    
    for(i = 1; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, num graphs generated (incl iso without colours): %llu\n", i, num_graphs_generated[i]);
    
    for(i = 1; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, num non-iso graphs gen col: %llu (incl. iso: %llu), noniso: %.2f%%\n", i,
                num_graphs_generated_noniso[i], num_graphs_generated_noniso[i] + num_graphs_generated_iso[i],
                (double) num_graphs_generated_noniso[i] / (num_graphs_generated_noniso[i] + num_graphs_generated_iso[i]) * 100);

    
    if(output_vertex_critical_graphs)
        fprintf(stderr, "Number of vertex-critical graphs written: %llu\n", num_graphs_written);
    else {
#ifdef TEST_IF_CRITICAL      
        fprintf(stderr, "Number of critical graphs written: %llu\n", num_graphs_written);
#else        
        fprintf(stderr, "Number of possible critical graphs written: %llu\n", num_graphs_written);
#endif
    }
    fprintf(stderr, "CPU time: %.1f seconds.\n", (double) (savetime / time_factor));

    return (EXIT_SUCCESS);
}