/**
 * critical_Pfree_graphs.c
 *
 * A generator for k-critical Pt-free graphs.
 *
 * The latest version of CriticalPfreeGraphs can be found here:
 * http://caagt.ugent.be/criticalpfree/
 * 
 * Author: Jan Goedgebeur (jan.goedgebeur@ugent.be)
 * In collaboration with: Maria Chudnovsky, Oliver Schaudt and Mingxian Zhong
 *
 * -----------------------------------
 * 
 * Copyright (c) 2014-2021 Ghent University
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
 * cc -DWORDSIZE=32 -DMAXN=WORDSIZE -O3 critical_Pfree_graphs.c planarity.c nautyW1.a -o critical_Pfree_graphs
 *
 * Or:
 *
 * cc -DWORDSIZE=64 -DMAXN=WORDSIZE -O3 critical_Pfree_graphs.c planarity.c nautyL1.a -o critical_Pfree_graphs
 *
 * -DWORDSIZE=32 is slightly faster, but limits the order of the graphs to 32.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include "nauty.h"
#include "planarity.h" /* which includes nauty.h */

#define VERSION "1.0 - Arpil 27 2015"


/*************************Defines and global variables*************************/

/******************************Configuration***********************************/

//If the number of vertices is large enough, the splitlevel should be at least MIN_SPLITLEVEL
#define MIN_SPLITLEVEL 9
//#define MIN_SPLITLEVEL 8

#define MAX_SPLITLEVEL 20

//Uncomment to start the construction from the K1 instead of from a limited set of startgraphs
#define LIMIT_STARTGRAPHS

//Uncomment to output only k-critical H-free graphs (is a bit slower)
//Else possibly also non-critical graphs might be output (so these graphs still need to be tested for criticality by afterwards)
#define TEST_IF_CRITICAL

//Choose the *best* sim element as the one with the "least number of children".
//In most cases it's best only to start from a certain level e.g. 10 and not from 0 (except in girth case?)
//#define LEVEL_LEAST_CHILDREN 20
//#define LEVEL_LEAST_CHILDREN 25
#define LEVEL_LEAST_CHILDREN MAXN //i.e. don't apply optimisation

//Only applying least children trick if (nv mod %d) == 0 (and nv >= LEVEL_LEAST_CHILDREN)
//Doesn't seem to help much, so best to leave it on 1
#define LEAST_CHILDREN_LEVEL_MOD 1

//This doesn't really help much, so best to leave it on INT_MAX
#define BREAK_VALUE_LEAST_CHILDREN INT_MAX //Don't test all possible extensions only the first BREAK_VALUE ones

//Uncomment to first call contains_expansion_without_any_children() before doing other expansions
//Uncommenting this is a lot slower, but also a lot fewer graphs generated (but doesn't terminate)
//#define TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN

//If TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN, then only applying this trick from the following number of vertices + 1
#define CONTAINS_EXPANSION_WITHOUT_CHILDREN_LEVEL 20

/* 
 * Instead of constructing all possible paths for the P-free test, first test if one of the
 * NUM_PREV_SETS previous forbidden induced paths is still a forbidden induced path.
 * (As nearly all of the generated graphs contain a forbidden induced path)
 * 5 seems to be a good trade-off for this.
 */
#define NUM_PREV_SETS 20 //20 seems to be a good compromise for this


//Uncomment to apply Px+P1+P1 free test instead of just Px+P1-free
//#define APPLY_EXTRA_P1_FREE_TEST

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

//Important: it is assumed that v1 < v2
#define NBH_IS_SUBSET_OF_COL_DOM_EDGE_REMOVED(a, b, v1, v2) ((*GRAPHROW1(nautyg, (a), MAXM) & *GRAPHROW1((*nautyg_colour_dominated_edge_removed)[v1][v2], (b), MAXM)) == *GRAPHROW1(nautyg, (a), MAXM))

//Important: it is assumed that v1 < v2 < v3
#define NBH_IS_SUBSET_OF_COL_DOM_TRIANGLE_REMOVED(a, b, v1, v2, v3) ((*GRAPHROW1(nautyg, (a), MAXM) & *GRAPHROW1((*nautyg_colour_dominated_triangle_removed)[v1][v2][v3], (b), MAXM)) == *GRAPHROW1(nautyg, (a), MAXM))

//size of multicode is nv + ne        
#define MAX_MUTLICODE_SIZE(a) ((a) + (a)*((a)-1)/2)

/***********************Methods for splay-tree*****************************/

//Methods for the splay-tree (see splay.c for more information). 

typedef struct sp {
    //graph canon_form[MAXN*MAXM];
    graph* canon_form; //Is a pointer, should use malloc for this; isn't really faster but uses fewer memory!
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

GRAPH current_graph_copy;

typedef unsigned char TRIANGLE[3];

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

graph nautyg_colour_dominated_all[MAXN*MAXM];

//Colour domination graph where a given vertex is removed
graph nautyg_colour_dominated_vertex_removed[MAXN][MAXN*MAXM];

//Using malloc because it's safer (can exit if not enough memory can be allocated)
//graph nautyg_colour_dominated_edge_removed[MAXN][MAXN][MAXN*MAXM];
graph (*nautyg_colour_dominated_edge_removed)[MAXN][MAXN][MAXN*MAXM];

//graph nautyg_colour_dominated_triangle_removed[MAXN][MAXN][MAXN][MAXN*MAXM];
graph (*nautyg_colour_dominated_triangle_removed)[MAXN][MAXN][MAXN][MAXN*MAXM];

//List of the similar vertices which were already destroyed
//Convention: first small_similar_vertex, then larger_similar_vertex
//Destroying one similar vertex pair per step, so max is MAXN
unsigned char destroyed_similar_vertices[MAXN][2];
static int num_destroyed_similar_vertices = 0;

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

unsigned int marks2[MAXN];
static int markvalue2 = MAXVAL;
#define RESETMARKS2 {int mki; if ((markvalue2 += 1) > MAXVAL) \
      { markvalue2 = 1; for (mki=0;mki<MAXN;++mki) marks2[mki]=0;}}
#define MARK2(v) marks2[v] = markvalue2
#define UNMARK2(v) marks2[v] = markvalue2 - 1
#define ISMARKED2(v) (marks2[v] == markvalue2)

/******************************************************************************/

int is_sim_pair = 0;

int valid_colouring_found_forced_colours = 0;

/******************************************************************************/

//For contains_forbidden_induced_path

//Only accepts graphs which do not contain induced paths of more than max_path_size vertices
//So only outputs Pn-free graphs with n = max_path_size+1
int max_path_size = 0;

static int current_path[MAXN];
static int current_pathsize = 0;
static setword current_path_bitvector;
static setword current_path_with_forbidden_neighbourhood;

static int longest_pathsize_found = 0;


static int disjoint_forbidden_path_found = 0;

int test_for_disjoint_paths = 0;
int max_path_size_disjoint_smaller = 0;

static int current_path_disjoint[MAXN];
static int current_pathsize_disjoint = 0;
static setword current_path_bitvector_disjoint;

static int longest_pathsize_found_disjoint = 0;

//TODO: temp bound!
#define MAX_NUM_CYCLES (MAXN*MAXN)

static int stored_cycles[MAX_NUM_CYCLES][MAXN];
static setword stored_cycles_bitvectors[MAX_NUM_CYCLES];
int num_cycles_stored = 0;


//Using malloc because it's safer (will exit if not enough memory can be allocated)
//Important: make sure everything is initialized to 0!
//static int previous_induced_path[MAXN+1][NUM_PREV_SETS][MAXN] = {0};
int (*previous_induced_paths)[MAXN+1][NUM_PREV_SETS][MAXN];

//For contains_forbidden_induced_cycle
int test_if_contains_forbidden_cycle = 0;
int forbidden_cycle_size = 0;

int test_if_diamondfree = 0;

int test_if_trianglefree = 0;

int test_if_K4free = 0;

int test_if_K5free = 0;

int test_if_planar = 0;

int test_girth = 0;
int girth = 0;

int start_from_inputgraphs = 0;

//If set to one vertex critical instead of critical graphs are output
int output_vertex_critical_graphs = 0;

//For is_critical()

typedef unsigned char EDGE[2];

#define MAX_EDGES ((MAXN)*((MAXN)-1)/2)

static EDGE edgelist[MAX_EDGES];
static int num_edges = 0;

static EDGE edgelist_temp[MAX_EDGES];
static int num_edges_temp = 0;
static int is_colourable_edge_removed[MAX_EDGES];

static int is_critical = 0;

static int test_if_contains_induced_fork = 0;

static int induced_fork_found = 0;

static int total_quadruples_found = 0;

static int test_if_contains_induced_bull = 0;

static int test_if_contains_induced_gem = 0;

// MA. Declare test if variable
static int test_if_contains_induced_copy = 0;

/******************************************************************************/

//For contains_W5 test
static unsigned long long int possible_cycle_vertices = 0;
static int contains_forbidden_cycle = 0;


//For contains cutvertex test
static int best_number_of_elements_in_component = MAXN;
static setword best_component = 0;
static setword current_component = 0;

static int number_of_vertices_marked = 0;

/******************************************************************************/

//Only accepts graphs which are NOT k-colourable (on last level)
int num_colours = 0;

int previous_extension_was_not_colourable = 0;

/* Specific parameters for modulo */
static int modulo = 1;
static int rest = 0;
static int splitcounter = 0;
static int splitlevel = 0; /* For modolo, determines at what level/depth of the recursion tree the calculation should be split */


//2*MAXN is not enough!
#define MAX_SIM_VERTICES (MAXN * MAXN / 2)

unsigned char similar_vertices[MAX_SIM_VERTICES][2];
int num_similar_vertices = 0;

unsigned char small_vertices[MAXN];
int num_small_vertices = 0;

//2*MAXN is not enough!
#define MAX_SIM_EDGES (MAXN * MAXN)

unsigned char similar_edges[MAX_SIM_EDGES][4];
int num_similar_edges = 0;

static int number_of_graphs_generated_from_parent = 0;
static int number_of_uncol_graphs_generated_from_parent = 0;

/******************************************************************************/

//For statistics
static unsigned long long int num_graphs_generated[MAXN + 1];
static unsigned long long int num_graphs_generated_iso[MAXN + 1];
static unsigned long long int num_graphs_generated_noniso[MAXN + 1];
static unsigned long long int num_graphs_written = 0;

static unsigned long long int counts_contains_diamond[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_diamond[MAXN + 1] = {0};

static unsigned long long int counts_contains_K4[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_K4[MAXN + 1] = {0};

static unsigned long long int counts_contains_triangle[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_triangle[MAXN + 1] = {0};

static unsigned long long int counts_contains_W5[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_W5[MAXN + 1] = {0};

static unsigned long long int counts_contains_K5[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_K5[MAXN + 1] = {0};

static unsigned long long int times_graph_colourable[MAXN + 1] = {0};
static unsigned long long int times_graph_not_colourable[MAXN + 1] = {0};

static unsigned long long int times_graph_vertex_critical[MAXN + 1] = {0};
static unsigned long long int times_graph_not_vertex_critical[MAXN + 1] = {0};

static unsigned long long int times_induced_path_found_previous[MAXN + 1] = {0};
static unsigned long long int times_induced_path_not_found_previous[MAXN + 1] = {0};

static unsigned long long int times_graph_Pfree[MAXN + 1] = {0};
static unsigned long long int times_graph_not_Pfree[MAXN + 1] = {0};

static unsigned long long int times_graph_forkfree[MAXN + 1] = {0};
static unsigned long long int times_graph_not_forkfree[MAXN + 1] = {0};

static unsigned long long int times_graph_bullfree[MAXN + 1] = {0};
static unsigned long long int times_graph_not_bullfree[MAXN + 1] = {0};

static unsigned long long int times_graph_gemfree[MAXN + 1] = {0};
static unsigned long long int times_graph_not_gemfree[MAXN + 1] = {0};

// MA. Create times graph (not) induced copy free
static unsigned long long int times_graph_induced[MAXN + 1] = {0};
static unsigned long long int times_graph_not_induced[MAXN + 1] = {0};

static unsigned long long int counts_contains_forbidden_induced_cycle[MAXN + 1] = {0};
static unsigned long long int counts_doesnt_contain_forbidden_induced_cycle[MAXN + 1] = {0};

static unsigned long long int times_contains_similar_vertices[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_vertices[MAXN+1] = {0};

static unsigned long long int times_contains_similar_vertices_col_dom[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_vertices_col_dom[MAXN+1] = {0};

static unsigned long long int times_contains_similar_edgepair[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_edgepair[MAXN+1] = {0};

static unsigned long long int times_contains_similar_edgepair_col_dom[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_edgepair_col_dom[MAXN+1] = {0};

static unsigned long long int times_contains_similar_triangle[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_triangle[MAXN+1] = {0};

static unsigned long long int times_contains_similar_diamond[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_diamond[MAXN+1] = {0};

static unsigned long long int times_contains_similar_P4[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_P4[MAXN+1] = {0};

static unsigned long long int times_contains_similar_P6[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_P6[MAXN+1] = {0};

static unsigned long long int times_contains_even_crown[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_even_crown[MAXN+1] = {0};

static unsigned long long int times_contains_odd_crown[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_odd_crown[MAXN+1] = {0};

static unsigned long long int times_contains_cutvertex[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_cutvertex[MAXN+1] = {0};

static unsigned long long int times_no_lemmas_applicable[MAXN+1] = {0};

static unsigned long long int times_contains_similar_triangle_col_dom[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_similar_triangle_col_dom[MAXN+1] = {0};

static unsigned long long int times_contains_small_vertex[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_small_vertex[MAXN+1] = {0};

static unsigned long long int times_contains_small_vertex_colour[MAXN+1] = {0};
static unsigned long long int times_doesnt_contain_small_vertex_colour[MAXN+1] = {0};

static unsigned long long int times_similar_element_destroyed[MAXN+1] = {0};
static unsigned long long int times_similar_element_not_destroyed[MAXN+1] = {0};

static unsigned long long int times_small_vertex_destroyed[MAXN+1] = {0};
static unsigned long long int times_small_vertex_not_destroyed[MAXN+1] = {0};

static unsigned long long int times_similar_vertices_are_still_destroyed[MAXN+1] = {0};
static unsigned long long int times_similar_vertices_no_longer_destroyed[MAXN+1] = {0};

static unsigned long long int times_graph_is_planar[MAXN+1] = {0};
static unsigned long long int times_graph_isnt_planar[MAXN+1] = {0};

static unsigned long long int times_no_children_sim_vert[MAXN+1] = {0};
static unsigned long long int times_children_sim_vert[MAXN+1] = {0};

static unsigned long long int times_no_children_sim_vert_col[MAXN+1] = {0};
static unsigned long long int times_children_sim_vert_col[MAXN+1] = {0};

static unsigned long long int times_no_children_small_vert[MAXN+1] = {0};
static unsigned long long int times_children_small_vert[MAXN+1] = {0};

static unsigned long long int times_no_children_small_vert_col[MAXN+1] = {0};
static unsigned long long int times_children_small_vert_col[MAXN+1] = {0};

static unsigned long long int times_no_children_sim_edge[MAXN+1] = {0};
static unsigned long long int times_children_sim_edge[MAXN+1] = {0};

static unsigned long long int times_no_children_sim_edge_col[MAXN+1] = {0};
static unsigned long long int times_children_sim_edge_col[MAXN+1] = {0};

static unsigned long long int times_no_children_odd_crown[MAXN+1] = {0};
static unsigned long long int times_children_odd_crown[MAXN+1] = {0};

static unsigned long long int times_no_children_even_crown[MAXN+1] = {0};
static unsigned long long int times_children_even_crown[MAXN+1] = {0};

static unsigned long long int times_expansion_with_no_children[MAXN+1] = {0};
static unsigned long long int times_no_expansion_with_no_children[MAXN+1] = {0};

/*********************************Methods**************************************/

static void extend(setword isolated_vertices, unsigned char avoided_vertex,
                   int max_colours_nbh_colouring, unsigned char destroyed_small_vertex, int perform_extensions);

int chromatic_number_same_colour_vertices(GRAPH graph, ADJACENCY adj, int genug,
                                          setword same_colour_vertices);

/******************************************************************************/

/**
 * Prints the given nauty graph in sparse representation.
 */
static void
print_sparse_graph_nauty(sparsegraph sparse_graph) {
    int i, j;
    fprintf(stderr, "Printing sparse graph nauty:\n");
    for(i = 0; i < sparse_graph.nv; i++) {
        fprintf(stderr, "%d :", i);
        for(j = 0; j < sparse_graph.d[i];j++) {
            //fprintf(stderr, " %d", sparse_graph.e[i * REG + j]);
            fprintf(stderr, " %d", sparse_graph.e[sparse_graph.v[i] + j]);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "Number of directed edges: %lu\n", (unsigned long) sparse_graph.nde);
}

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
    return memcmp(canong, list->canon_form, nv * sizeof (graph));
    //return memcmp(canong, list->canon_form, MAXN * sizeof (graph)); //Is a lot slower!
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

static void
add_edge_zz(GRAPH current_graph, unsigned char adj[], unsigned char v, unsigned char w) {
    current_graph[v][adj[v]] = w;
    current_graph[w][adj[w]] = v;
    adj[v]++;
    adj[w]++;
}


/******************************************************************************/

static void
decode_multicode_zz(unsigned char code[], GRAPH current_graph, ADJACENCY adj, int codelength) {
    int i;

    if(nv != code[0]) {
        fprintf(stderr, "Error: Wrong number of vertices: expected %d while found %d vertices \n", nv, code[0]);
        exit(1);
    }

    for(i = 0; i < nv; i++) {
        adj[i] = 0;
        //neighbours[i] = 0;
    }

    int currentvertex = 1;
    for(i = 1; i < codelength; i++) {
        if(code[i] == 0) {
            currentvertex++;
        } else {
            add_edge_zz(current_graph, adj, currentvertex - 1, code[i] - 1);
        }
    }

    for(i = 0; i < nv; i++) {
        if(adj[i] > MAXN - 1) {
            fprintf(stderr, "Error: graph can have at most %d neighbours, but found %d neighbours\n",
                    MAXN - 1, adj[i]);
            exit(1);
        }
    }

}


//Outputs the graph to stdout in multicode
static void
output_graph(GRAPH g, ADJACENCY adj, int num_vertices) {

    printf("%s", "\n\nAdjacency 'matrix': ");

    // Loop through the array and print the values
    for (int i = 0; i <= num_vertices; i++) {
        printf("%u ", adj[i]);
    }
    
        printf("%s", "\ncode_multicode: ");

    unsigned char codebuffer[MAX_MUTLICODE_SIZE(num_vertices)];
    int codelength = code_multicode(codebuffer, g, adj, num_vertices);

    // ./critical_Pfree_graphs 10 c4 P5
        printf("%s", "\n Code Length \n");

    printf("%d", codelength);


    // Define the file path
    char file_path[] = "./my_outputs.txt";

    // Reference the file pointer
    fopen(file_path, "w"); // Clear file to begin with
    FILE * fp = fopen(file_path, "a"); // Append to file afterwards

    // Simple, loopy loopy
    int c;
    for( c = 0; c < codelength; c = c + 1 ){

        // Save the codebuffer at [a] to file
        fprintf(fp, "%hhx", codebuffer[c]);

        // Add space to characters
        fprintf(fp, "%s", " ");

        // print out to terminal
        printf("%d", codebuffer[c]);

        // Add space to characters
        printf("%s", " ");

    }

    // New line after writing output
    fprintf(fp, "%s ", "\n");

    printf("%s", "\n Before \n");

    // Get the # of rows in the graph
    size_t graph_row_length = sizeof(g);
    printf("%s", "\n# of Rows: ");
    printf("%ld", graph_row_length);
    printf("%s", "\n[\n");

    // For every row
    int a;
    for(a = 0; a < graph_row_length - 1; a++) {

        // Get the length of each row (columns)
        size_t graph_column_length = sizeof(g[a]);

        printf("%s", "\n\t# of Columns: ");
        printf("%ld", graph_column_length);
        printf("%s", "\n");

        printf("%s", "\t[");

        int b;
        for(b = 0; b < graph_column_length - 1; b++) {

            printf("%d", g[a][b]);
            printf("%s", " ");

        }

        printf("%s", "]\n");

    }

    printf("%s", "]\n\n");


    // New line even in terminal prinout
    // printf("%s", "\n");

    // Increment num of graphs written
    num_graphs_written++;
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

                    setword same_colour_vertices = removed_vertices;
                    ADDELEMENT1(&same_colour_vertices, i);
                    ADDELEMENT1(&same_colour_vertices, j);

                    if(chromatic_number_same_colour_vertices(current_graph_mb, adj_mb, num_colours,
                                                             same_colour_vertices) == 0) {
                        //i.e. there is no valid colouring where i and j have the same colour
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
    for(removed_vertex = 0; removed_vertex < nv; removed_vertex++) {
        if(!ISMARKED2(removed_vertex)) { //If already marked then was already determined!
            compute_colour_domination_graphs_vertex_removed_given(removed_vertex);
        }
    }

}

/******************************************************************************/

/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - e.
 */
static void
compute_colour_domination_graphs_edge_removed() {
    int k, l;
    for(k = 0; k < nv; k++)
        for(l = 0; l < adj[k]; l++)
            if(k < current_graph[k][l]) {
                //Remove edge k current_graph[k][l]
                setword removed_vertices = 0;
                ADDELEMENT1(&removed_vertices, k);
                ADDELEMENT1(&removed_vertices, current_graph[k][l]);

                make_graph_with_removed_vertices(removed_vertices);

                copy_nauty_graph(current_graph, adj, nv, (*nautyg_colour_dominated_edge_removed)[k][current_graph[k][l]]);
                search_for_hidden_edges(removed_vertices, (*nautyg_colour_dominated_edge_removed)[k][current_graph[k][l]]);
            }

}

/******************************************************************************/

/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - triangle.
 */
static void
compute_colour_domination_graphs_triangle_removed() {
    int a, b, c;
    int j;
    setword neighbourhood_a_b;
    for(a = 0; a < nv; a++) {
        for(j = 0; j < adj[a]; j++) {
            b = current_graph[a][j];
            if(a < b) {
                neighbourhood_a_b = *GRAPHROW1(nautyg, a, MAXM) & *GRAPHROW1(nautyg, b, MAXM);
                c = b; //since we want that c > b
                while((c = nextelement1(&neighbourhood_a_b, 1, c)) >= 0) {
                    //triangle a < b < c formed
                    setword removed_vertices = 0;
                    ADDELEMENT1(&removed_vertices, a);
                    ADDELEMENT1(&removed_vertices, b);
                    ADDELEMENT1(&removed_vertices, c);

                    make_graph_with_removed_vertices(removed_vertices);

                    copy_nauty_graph(current_graph, adj, nv, (*nautyg_colour_dominated_triangle_removed)[a][b][c]);
                    search_for_hidden_edges(removed_vertices, (*nautyg_colour_dominated_triangle_removed)[a][b][c]);
                }
            }
        }
    }
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

/**
 * Returns 1 if current_set contains at least one element of forced_vertices_at_least_one,
 * else returns 0.
 * Also returns 0 if forced_vertices_at_least_one is empty
 */
static int
contains_forced_vertices_at_least_one(setword current_set_bitvector,
                                      setword forced_vertices_at_least_one) {
    if(forced_vertices_at_least_one == 0)
        return 1;
    else
        return (current_set_bitvector & forced_vertices_at_least_one) != 0;
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

//Returns 1 if the graph contains a 4-cycle which also contains the last vertex, else returns 0
//Important: the 4-cycle is not necessarily induced!
static int
countains_fourcycle_without_expand(unsigned char current_set[], int current_set_size) {
    int i, j;
    for(i = 0; i < current_set_size - 1; i++)
        for(j = i + 1; j < current_set_size; j++)
            if((nautyg[current_set[i]] & nautyg[current_set[j]]) != 0)
                return 1; //Vertices have a common neighbour so together with the new vertex this will form a 4-cycle

    return 0;
}

/******************************************************************************/

//In case of similar vertices: num possible expansions = 2^(n-2)
static void
construct_and_expand_vertex_sets_forbidden_vertices(unsigned char current_set[], int current_set_size, setword current_set_bitvector,
                                                    int next_vertex, setword forbidden_vertices, setword isolated_vertices, unsigned char avoided_vertex,
                                                    int max_colours_nbh_colouring, unsigned char destroyed_small_vertex,
                                                    setword forced_vertices_at_least_one, int perform_extensions) {
    if(current_set_size > 0 && contains_forced_vertices_at_least_one(current_set_bitvector, forced_vertices_at_least_one)) {
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

        //Still need to test this since startvertices may already contain a C4!
        if(test_girth && girth > 4 && countains_fourcycle_without_expand(current_set, current_set_size)) {
            //Larger expansions will certainly also contain a C4!
            //fprintf(stderr, "C4 found!\n");
            return;
        }

        //So perform expansion here
        expand_vertex_set_no_remove(current_set, current_set_size);

        extend(isolated_vertices, avoided_vertex, max_colours_nbh_colouring, destroyed_small_vertex, perform_extensions);

        //reduce
        reduce_vertex_set_no_remove(current_set, current_set_size);

        if(previous_extension_was_not_colourable) {
            //So the next graph also won't be colourable because it contains the same edges
            //But this graph can't be (edge)-critical since it contains more edges
            //So we can already stop here

            //fprintf(stderr, "nv: %d, size: %d\n", nv, current_set_size);

            previous_extension_was_not_colourable = 0;

            return;
        }
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
                if(test_girth && girth > 4) {
                    //Neighbours of neighbours are also forbidden, else there would be a 4-cycle
                    int j;
                    for(j = 0; j < adj[i]; j++)
                        forbidden_vertices |= nautyg[current_graph[i][j]];
                }
            }

            construct_and_expand_vertex_sets_forbidden_vertices(current_set, current_set_size + 1, current_set_bitvector,
                                                                i + 1, forbidden_vertices, isolated_vertices, avoided_vertex,
                                                                max_colours_nbh_colouring, destroyed_small_vertex, forced_vertices_at_least_one, perform_extensions);

            DELELEMENT1(&current_set_bitvector, i);

            if(test_if_trianglefree) {
                forbidden_vertices = forbidden_vertices_backup;
                //forbidden_vertices &= ~nautyg[i]; //Is slower!
            }
        }
    }

}

/******************************************************************************/

void colournext(GRAPH graph, int *colours, int tiefe, int *minsofar, int usedsofar,
                unsigned int forbidden[], int *nofc, unsigned int *MASK, int genug, int *good_enough)
{
    int changed[MAXN+1];
    /* gibt an, ob die nofc und forbidden werte geaendert wurden */
    int maxforbidden, vertex, i, colour, nachbar, stillok;

//for (i=2; colours[i]; i++); /* suche ersten nicht gefaerbten Wert */
    for (i=1; colours[i]; i++); //Now must start from 1, since first vertex may not be coloured already by chromatic_number_same_colour_vertices()
    maxforbidden=nofc[i]; vertex=i;
    for ( ; (i<=graph[0][0]) && (maxforbidden<*minsofar-2); i++)
        if ((nofc[i]>maxforbidden) && !colours[i])
        { maxforbidden=nofc[i]; vertex=i; }
/* suche Wert mit maximaler Anzahl von verbotenen Farben, der noch nicht
   gefaerbt ist */


    for ( colour=1; (colour<=usedsofar+1) && (colour < *minsofar)
                    && (usedsofar < *minsofar) && !(*good_enough); colour++ )
    {
        if ( !(forbidden[vertex] & MASK[colour]) )
        { colours[vertex]=colour;
            stillok=1;
            for (i=0; (nachbar=graph[vertex][i])!=leer; i++)
                if (!(MASK[colour] & forbidden[nachbar]))
                { nofc[nachbar]++;
                    if ((nofc[nachbar]>= *minsofar-1) && (!colours[nachbar]))
                        stillok=0;
                    forbidden[nachbar] |= MASK[colour];
                    changed[nachbar]=1; } else changed[nachbar]=0;
            if (tiefe==graph[0][0])
            { if (colour==usedsofar+1) *minsofar=colour;
                else *minsofar=usedsofar;
                if (genug) *good_enough=1;}
            else /* d.h. tiefe < graph[0][0] */
            if (stillok)
            { if (colour==usedsofar+1)
                    colournext(graph,colours,tiefe+1,minsofar,colour,forbidden,nofc,MASK,genug,good_enough);
                else
                    colournext(graph,colours,tiefe+1,minsofar,usedsofar,forbidden,nofc,MASK,genug,good_enough);
            }
            colours[vertex]=0;
            for (i=0; (nachbar=graph[vertex][i])!=leer; i++)
                if (changed[nachbar])
                { nofc[nachbar]--;
                    forbidden[nachbar] &= ~MASK[colour]; }
        }
    } /* Ende grosse for-schleife */
}

/******************************************************************************/


/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
int chromatic_number(GRAPH graph, ADJACENCY adj, int genug) {
    unsigned int MASK[32];
    unsigned int forbidden[MAXN+1]; /*Die verbotenen Farben als int-menge*/
    int nofc[MAXN+1]; /* number of forbidden colours */
    int colours[MAXN+1], minadj, maxadj, minsofar, nachbar, puffer;
    int i, good_enough;

    good_enough=0;

    MASK[0]=1;
    for (i=1;i<32;i++) MASK[i]=MASK[i-1]<<1;


    minadj=graph[0][0]; maxadj=0;
    for (i=1; i<=graph[0][0]; i++)
    { forbidden[i]=nofc[i]=colours[i]=0;
        if (adj[i]<minadj) minadj=adj[i]; if (adj[i]>maxadj) maxadj=adj[i];
    }

    if (minadj==graph[0][0]-1)
    { if ((genug==0) || (genug>=graph[0][0])) return(graph[0][0]);
            /* es war der vollstaendige graph */
        else return(0); }
//else minsofar=maxadj; //Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
    else minsofar=maxadj+1;


    if (genug && (genug < minsofar)) minsofar=genug+1; /* Es wird einfach so getan, als haette man schon
					   so eine Faerbung (wuerde ja eh nichts nuetzen */

    if (minsofar>=32) { printf("Too many possible colours for the colouring function\n");
        exit(1);
    }

//Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
/*
if ((minadj==2) && (maxadj==2))
    { if (genug==1) return(0);
      if ((graph[0][0]%2)==0) return(2); else
	{ if (genug!=2) return(3); else return(0); } }
*/

    colours[1]=1;
    for (i=0; (nachbar=graph[1][i])!=leer; i++)
    { nofc[nachbar]++; forbidden[nachbar] |= MASK[1]; }
    puffer=graph[1][0];
//Warning: added. Is necessary since first vertex might be isolated!
    if(puffer == leer) {
        if(graph[0][0] < 2) {
            fprintf(stderr, "Error: graph must have at least 2 vertices\n");
            exit(1);
        }
        puffer = 2;
        //In this case it doesn't matter what colour this vertex gets
        //as it has no neighbours
        if(graph[0][0] == 2)
            return 1; //Special case else the method returns chrom num 0?
    }
/* Der erste Nachbar bekommt Farbe 2*/
    colours[puffer]=2;
    for (i=0; (nachbar=graph[puffer][i])!=leer; i++)
    { nofc[nachbar]++; forbidden[nachbar] |= MASK[2]; }


    colournext(graph,colours,3,&minsofar,2,forbidden,nofc,MASK,genug,&good_enough);

    if ((genug==0) || (minsofar<=genug)) return(minsofar); else return(0);
}

/******************************************************************************/

/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
//In this modified method all vertices in same_colour_vertices get colour 1.
//Important: vertices are labelled from 0 in same_colour_vertices (not from 1)!
int chromatic_number_same_colour_vertices(GRAPH graph, ADJACENCY adj, int genug,
                                          setword same_colour_vertices) {
    unsigned int MASK[32];
    unsigned int forbidden[MAXN+1]; /*Die verbotenen Farben als int-menge*/
    int nofc[MAXN+1]; /* number of forbidden colours */
    int colours[MAXN+1], minadj, maxadj, minsofar, nachbar;
    int i,j, good_enough;

    good_enough=0;

    MASK[0]=1;
    for (i=1;i<32;i++) MASK[i]=MASK[i-1]<<1;


    minadj=graph[0][0]; maxadj=0;
    for (i=1; i<=graph[0][0]; i++)
    { forbidden[i]=nofc[i]=colours[i]=0;
        if (adj[i]<minadj) minadj=adj[i]; if (adj[i]>maxadj) maxadj=adj[i];
    }

    if (minadj==graph[0][0]-1)
    { if ((genug==0) || (genug>=graph[0][0])) return(graph[0][0]);
            /* es war der vollstaendige graph */
        else return(0); }
//else minsofar=maxadj; //Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
    else minsofar=maxadj+1;


    if (genug && (genug < minsofar)) minsofar=genug+1; /* Es wird einfach so getan, als haette man schon
					   so eine Faerbung (wuerde ja eh nichts nuetzen */

    if (minsofar>=32) { printf("Too many possible colours for the colouring function\n");
        exit(1);
    }

//Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
/*
    if ((minadj==2) && (maxadj==2))
        { if (genug==1) return(0);
          if ((graph[0][0]%2)==0) return(2); else
            { if (genug!=2) return(3); else return(0); } }
     */

    //Give same_colour_vertices colour 1
    int num_vertices_already_coloured = 0;
    for(i = 1; i <= graph[0][0]; i++) {
        if(ISELEMENT1(&same_colour_vertices, i - 1)) {
            colours[i] = 1;
            num_vertices_already_coloured++;
            for(j = 0; (nachbar = graph[i][j]) != leer; j++) {
                if(!(MASK[1] & forbidden[nachbar])) { //Since same colour vertices may have same neighbour
                    nofc[nachbar]++;
                    forbidden[nachbar] |= MASK[1];
                }
            }
        }
    }

    //Necessary for is_valid_crown() test since all vertices might already be coloured!
    if(num_vertices_already_coloured == graph[0][0])
        return 1;

    colournext(graph,colours,num_vertices_already_coloured+1,&minsofar,1,forbidden,nofc,MASK,genug,&good_enough);

    if ((genug==0) || (minsofar<=genug)) return(minsofar); else return(0);
}

/******************************************************************************/

/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
//In this modified method all vertices in vertices[] get colour given_colours[i]
//Important: vertices are labelled from 0 in given_colours[] and colours start from 1 in given_colours[]
//Also important: it is assumed that the startcolouring is a valid colouring
int chromatic_number_given_startcolouring(GRAPH graph, ADJACENCY adj, int genug,
                                          unsigned char vertices[], int vertices_size, unsigned char given_colours[]) {
    unsigned int MASK[32];
    unsigned int forbidden[MAXN+1]; /*Die verbotenen Farben als int-menge*/
    int nofc[MAXN+1]; /* number of forbidden colours */
    int colours[MAXN+1], minadj, maxadj, minsofar, nachbar;
    int i,j, good_enough;

    good_enough=0;

    MASK[0]=1;
    for (i=1;i<32;i++) MASK[i]=MASK[i-1]<<1;


    minadj=graph[0][0]; maxadj=0;
    for (i=1; i<=graph[0][0]; i++)
    { forbidden[i]=nofc[i]=colours[i]=0;
        if (adj[i]<minadj) minadj=adj[i]; if (adj[i]>maxadj) maxadj=adj[i];
    }

    if (minadj==graph[0][0]-1)
    { if ((genug==0) || (genug>=graph[0][0])) return(graph[0][0]);
            /* es war der vollstaendige graph */
        else return(0); }
//else minsofar=maxadj; //Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
    else minsofar=maxadj+1;


    if (genug && (genug < minsofar)) minsofar=genug+1; /* Es wird einfach so getan, als haette man schon
					   so eine Faerbung (wuerde ja eh nichts nuetzen */

    if (minsofar>=32) { printf("Too many possible colours for the colouring function\n");
        exit(1);
    }

//Warning: modified so theorem brooks is not applied anymore so this also works for disconnected graphs!
/*
    if ((minadj==2) && (maxadj==2))
        { if (genug==1) return(0);
          if ((graph[0][0]%2)==0) return(2); else
            { if (genug!=2) return(3); else return(0); } }
     */

    //Give vertices from vertices[] colour given_colours[i]
    setword colours_used_so_far = 0;
    for(i = 0; i < vertices_size; i++) {
        if((MASK[given_colours[i]] & forbidden[vertices[i] + 1])) {
            fprintf(stderr, "Error: invalid startcolouring!\n");
            exit(1);
        }
        colours[vertices[i] + 1] = given_colours[i];
        ADDELEMENT1(&colours_used_so_far, given_colours[i]);
        for(j = 0; (nachbar = graph[vertices[i] + 1][j]) != leer; j++) {
            if(!(MASK[given_colours[i]] & forbidden[nachbar])) { //Since same colour vertices may have same neighbour
                nofc[nachbar]++;
                forbidden[nachbar] |= MASK[given_colours[i]];
            }
        }
    }

    //Now also colour isolated vertices with colour 1
    for(i = 1; i <= nv; i++)
        if(adj[i] == 0) {
            colours[i] = 1;
            vertices_size++;
            if(!ISELEMENT1(&colours_used_so_far, 1)) {
                fprintf(stderr, "Error: colour one was not used before!\n");
                exit(1);
            }
        }

    colournext(graph,colours,vertices_size+1,&minsofar,POPCOUNT(colours_used_so_far),forbidden,nofc,MASK,genug,&good_enough);

    if ((genug==0) || (minsofar<=genug)) return(minsofar); else return(0);
}

/******************************************************************************/

//Returns 1 if current_graph is k-colourable, else returns 0
static int
is_colourable() {
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

    return chromatic_number(current_graph_mb, adj_mb, num_colours) > 0;

}

/******************************************************************************/

//It is assumed that current_pathlength > 0
//Determines induced paths which do not contain any vertices from current_path_with_forbidden_neighbourhood
static void
determine_longest_disjoint_induced_path_recursion(int maxlength) {
    if(current_pathsize_disjoint > longest_pathsize_found_disjoint) {
        longest_pathsize_found_disjoint = current_pathsize_disjoint;
        if(longest_pathsize_found_disjoint > maxlength)
            return;
    }

    int previous_vertex = current_path_disjoint[current_pathsize_disjoint - 1];
    setword previous_vertex_bitvector = 0;
    ADDELEMENT1(&previous_vertex_bitvector, previous_vertex);

    //set *gv = GRAPHROW1(nautyg, previous_vertex, MAXM);
    //Is a lot faster when using current_graph instead of nextelement1!
    //while((neighbour = nextelement1(gv, 1, neighbour)) >= 0) {
    int i;
    for(i = 0; i < adj[previous_vertex]; i++) {
        int neighbour = current_graph[previous_vertex][i];
        if(!ISELEMENT1(&current_path_with_forbidden_neighbourhood, neighbour)) { //Path must be disjoint
            set *gv_neighbour = GRAPHROW1(nautyg, neighbour, MAXM);
            if((current_path_bitvector_disjoint & *gv_neighbour) == previous_vertex_bitvector //Is the bottleneck but can't seem to get this faster?
               && neighbour != current_path_disjoint[0]) {
                //&& (BIT(next_vertex) & current_path_bitvector) == 0) {
                //&& !ISMARKED(next_vertex)) {
                current_path_disjoint[current_pathsize_disjoint++] = neighbour;
                ADDELEMENT1(&current_path_bitvector_disjoint, neighbour);

                //recursion
                determine_longest_disjoint_induced_path_recursion(maxlength);

                //Is slightly faster with this included
                if(longest_pathsize_found_disjoint > maxlength)
                    return;

                current_pathsize_disjoint--;
                DELELEMENT1(&current_path_bitvector_disjoint, neighbour);
            }
        }
    }

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

/**
 * Returns 1 if the graph contains an induced path with a given size where none of
 * the vertices of the path is in forbidden_vertices, else returns 0.
 */
static int contains_disjoint_forbidden_path(setword forbidden_vertices) {
    current_path_with_forbidden_neighbourhood = forbidden_vertices;

    if(max_path_size_disjoint_smaller > 1) {
        longest_pathsize_found_disjoint = 0;
        int i;
        for(i = 0; i < nv; i++)
            if(!ISELEMENT1(&current_path_with_forbidden_neighbourhood, i)) { //Path must be disjoint
                current_path_disjoint[0] = i;
                current_pathsize_disjoint = 1;
                current_path_bitvector_disjoint = 0;
                ADDELEMENT1(&current_path_bitvector_disjoint, i);

                //recursion
                determine_longest_disjoint_induced_path_recursion(max_path_size_disjoint_smaller);
                if(longest_pathsize_found_disjoint > max_path_size_disjoint_smaller) {
                    return 1;
                }
            }
    } else if(max_path_size_disjoint_smaller == 1) {
        //Test if there is an induced P2
        int i, j;
        for(i = 0; i < nv; i++)
            if(!ISELEMENT1(&current_path_with_forbidden_neighbourhood, i)) {
                for(j = 0; j < adj[i]; j++) {
                    int neighbour = current_graph[i][j];
                    if(i < neighbour && !ISELEMENT1(&current_path_with_forbidden_neighbourhood, neighbour)) {
                        //Disjoint P2 found!
                        return 1;
                    }
                }
            }
    } else if(max_path_size_disjoint_smaller == 0) {

#ifndef APPLY_EXTRA_P1_FREE_TEST
        //Test if there is a vertex which is not in current_path_bitvector
        //possible_cycle_vertices = ALLMASK(nv);
        //if((ALLMASK(nv) & ~current_path_bitvector) != 0) { //Geen verschil...
        if(current_path_with_forbidden_neighbourhood != ALLMASK(nv)) //A little bit faster...
            return 1;
#else
        //No, now test P_t + P_1 + P_1
        //So check if there are 2 vertices which are not in current_path_with_forbidden_neighbourhood
        //And such that the 2 vertices are no neighbours...
        setword candidate_vertices = (ALLMASK(nv) & ~current_path_with_forbidden_neighbourhood);
        if(POPCOUNT(candidate_vertices) <= 1)
            return 0;

        int i, j;
        for(i = 0; i < nv - 1; i++)
            if(ISELEMENT1(&candidate_vertices, i))
                for(j = i + 1; j < nv; j++)
                    if(ISELEMENT1(&candidate_vertices, j) && !ISELEMENT1(&nautyg[i], j))
                        return 1; //P1 + P1 found!

#endif

    }

    return 0;
}

/******************************************************************************/

//It is assumed that current_pathlength > 0
static void
determine_longest_induced_path_recursion(int maxlength) {
    if(current_pathsize > longest_pathsize_found) {
        int longest_pathsize_previous = longest_pathsize_found;
        longest_pathsize_found = current_pathsize;
        if(longest_pathsize_found > maxlength) {
            if(!test_for_disjoint_paths)
                store_current_path();
            else {
                setword forbidden_vertices = current_path_bitvector;
                //Neighbourhood of first induced path is also forbidden!
                int i;
                for(i = 0; i < current_pathsize; i++)
                    forbidden_vertices |= nautyg[current_path[i]];

                if(contains_disjoint_forbidden_path(forbidden_vertices)) {
                    disjoint_forbidden_path_found = 1;
                    store_current_path();
                } else {
                    //No disjoint induced path found
                    longest_pathsize_found = longest_pathsize_previous;
                }
            }

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
            if((!test_for_disjoint_paths || disjoint_forbidden_path_found) && longest_pathsize_found > maxlength)
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

    if(test_for_disjoint_paths) {
        setword forbidden_vertices = current_path_bitvector;
        //Neighbourhood of first induced path is also forbidden!
        //for(i = 0; i <= maxlength; i++)
        for(i = 0; i <= maxlength; i++)
            //forbidden_vertices |= *GRAPHROW1(nautyg, possible_induced_path[i], MAXM);
            forbidden_vertices |= nautyg[possible_induced_path[i]];

        return contains_disjoint_forbidden_path(forbidden_vertices);

    } else
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
    disjoint_forbidden_path_found = 0;
    for(i = 0; i < nv; i++) {
        //for(i = n-1; i >= 0; i--) { //doesn't make any difference
        current_path[0] = i;
        current_pathsize = 1;
        current_path_bitvector = 0;
        ADDELEMENT1(&current_path_bitvector, i);

        //recursion
        determine_longest_induced_path_recursion(max_path_size);
        if((!test_for_disjoint_paths || disjoint_forbidden_path_found) && longest_pathsize_found > max_path_size)
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

//Warning: 4-cycle is not necessarily induced!!!
static int
countains_fourcycle() {
    //Moet enkel naar laatste top kijken!
    setword last_vertex_bitvector = 0;
    ADDELEMENT1(&last_vertex_bitvector, nv - 1);
    int i, j;
    for(i = 0; i < adj[nv - 1] - 1; i++)
        for(j = i + 1; j < adj[nv - 1]; j++)
            if((nautyg[current_graph[nv - 1][i]] & nautyg[current_graph[nv - 1][j]]) != last_vertex_bitvector)
                //I.e. there are 2 neighbours of nv - 1 which have more than 1 common neighbour
                return 1;

    return 0;
}

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

//Returns 1 if the graph contains a W5 which also contains the last vertex, else returns 0
//The last vertex must be in the W5, since the parents were W5-free
//int n: nv of the graph
//Important: it is assumed that the graph is K4-free, otherwise this might give wrong results
static int
contains_W5() {
    contains_forbidden_cycle = 0;
    //RESETMARKS;

    //Option 1: test if last vertex is the center of the W5
    if(adj[nv - 1] >= 5) {
        int i;
        possible_cycle_vertices = *GRAPHROW1(nautyg, nv - 1, MAXM);
        //Doesn't have to do this for all neighbours since at least 5 neighbours will be on the cycle
        for(i = 0; i < adj[nv - 1] - 4; i++) {
            //Test if it is the center of a W5:
            //Test if it's neighbours form a C5
            current_path[0] = current_graph[nv - 1][i];
            current_pathsize = 1;
            current_path_bitvector = 0;
            ADDELEMENT1(&current_path_bitvector, current_graph[nv - 1][i]);
            //MARK(current_graph[n - 1][i]);
            DELELEMENT1(&possible_cycle_vertices, current_graph[nv - 1][i]);

            //Opmerking: moet zelfs niet perse testen of induced is, want aangezien W5 en K4-free kan enkel induced C5 bevatten...
            //Maar is sneller om induced te testen (omdat dit restrictiever is)
            contains_forbidden_induced_cycle_recursion(5);

            if(contains_forbidden_cycle)
                return 1;

            //UNMARK(current_graph[n - 1][i]);
            ADDELEMENT1(&possible_cycle_vertices, current_graph[nv - 1][i]);
        }
    }

    //Option 2: test if last vertex is in the C5
    //Slightly slower if testing option 2 first
    int i;
    for(i = 0; i < adj[nv-1];i++)
        if(adj[current_graph[nv-1][i]] >= 5) {
            possible_cycle_vertices = *GRAPHROW1(nautyg, current_graph[nv - 1][i], MAXM);
            current_path[0] = nv-1;
            current_pathsize = 1;
            current_path_bitvector = 0;
            ADDELEMENT1(&current_path_bitvector, nv-1);
            //MARK(n-1);
            DELELEMENT1(&possible_cycle_vertices, nv-1);

            //Opmerking: moet zelfs niet perse testen of induced is, want aangezien W5 en K4-free kan enkel induced C5 bevatten
            //Maar is sneller om induced te testen (omdat dit restrictiever is)
            contains_forbidden_induced_cycle_recursion(5);

            if(contains_forbidden_cycle)
                return 1;

            //UNMARK(n-1);
        }

    return 0;
}


/******************************************************************************/

//Returns 1 if the graph contains a K5 which also contains the last vertex, else returns 0
//The last vertex must be in the K5, since the parents were K5-free
//int n: nv of the graph
static int
contains_K5() {
    if(adj[nv-1] < 4) //Won't happen very often
        return 0;

    //int i;
    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        //for(i = 0; i < adj[n-1]; i++) {
        //setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, current_graph[n-1][i], MAXM) & *GRAPHROW1(nautyg, n-1, MAXM);
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        //P_POPCOUNT() does not make any difference as this is no bottleneck!
        if(POPCOUNT(intersect_neigh_n_b1) >= 3) {
            int neighbour_b2 = -1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b2)) >= 0) {
                setword intersect_neigh_n_b1_b2 = intersect_neigh_n_b1 & *GRAPHROW1(nautyg, neighbour_b2, MAXM);
                if(POPCOUNT(intersect_neigh_n_b1_b2) >= 2) {
                    int neighbour_b3 = -1;
                    while((neighbour_b3 = nextelement1(&intersect_neigh_n_b1_b2, 1, neighbour_b3)) >= 0)
                        if((intersect_neigh_n_b1_b2 & *GRAPHROW1(nautyg, neighbour_b3, MAXM)) != 0)
                            return 1; //Contains K5
                }
            }
        }
    }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a K4 which also contains the last vertex, else returns 0
//The last vertex must be in the K4, since the parents were K4-free
//int n: nv of the graph
static int
contains_K4() {
    if(adj[nv-1] < 3) //Won't happen very often
        return 0;

    //int i;
    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        //for(i = 0; i < adj[n-1]; i++) {
        //setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, current_graph[n-1][i], MAXM) & *GRAPHROW1(nautyg, n-1, MAXM);
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        //P_POPCOUNT() does not make any difference as this is no bottleneck!
        if(POPCOUNT(intersect_neigh_n_b1) >= 2) {
            int neighbour_b2 = -1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b2)) >= 0)
                if((intersect_neigh_n_b1 & *GRAPHROW1(nautyg, neighbour_b2, MAXM)) != 0)
                    return 1; //Contains K4
        }
    }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a diamond which also contains the last vertex, else returns 0
//The last vertex must be in the diamond, since the parents were diamond-free
//int n: nv of the graph
//Important: this method just tests if the graph contains a diamond as a subgraph,
//not necessarily as an induced subgraph (so it could contain a diamond in a K4)!
static int
contains_diamond() {
    if(adj[nv-1] < 2) //Won't happen very often
        return 0;

    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        int popcnt = POPCOUNT(intersect_neigh_n_b1);
        if(popcnt >= 2)
            return 1; //contains diamond; n is a degree 3 vertex of the diamond
        else if(popcnt == 1) {
            //Find third vertex of the triangle
            int neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, -1);
            if(POPCOUNT(*GRAPHROW1(nautyg, neighbour_b1, MAXM) & *GRAPHROW1(nautyg, neighbour_b2, MAXM)) >= 2)
                return 1; //contains diamond; n is a degree 2 vertex of the diamond
        }
    }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a diamond which also contains the last vertex, else returns 0
//The last vertex must be in the diamond, since the parents were diamond-free
//int n: nv of the graph
static int
contains_diamond_induced() {

    if(adj[nv-1] < 2) //Won't happen very often
        return 0;


    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        int popcnt = POPCOUNT(intersect_neigh_n_b1);
        if(popcnt >= 2) { //n-1 b1 center edge of diamond
            //  return 1; //contains diamond; n is a degree 3 vertex of the diamond
            //Nee, niet noodzakelijk induced!!!
            int neighbour_b1_n_1 = -1;
            while((neighbour_b1_n_1 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b1_n_1)) >= 0) {
                int neighbour_b1_n_2 = neighbour_b1_n_1;

                while((neighbour_b1_n_2 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b1_n_2)) >= 0) {
                    //Als neighbour_b1_n_1 geen buur van neighbour_b1_n_2, dan ret 1 (dwz induced diamond)

                    if(neighbour_b1_n_2 == neighbour_b1_n_1) {
                        fprintf(stderr, "Error: shouldn't happen!\n");
                        exit(1);
                    }

                    if(!ISELEMENT1(&nautyg[neighbour_b1_n_1], neighbour_b1_n_2)) {
                        return 1; //Induced diamond
                    }
                }
            }
        }
        //else if(popcnt == 1) {
        if(popcnt >= 1) { //n-1 b1 corner edge of diamond
            //Find third vertex of the triangle
            //int neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, -1);
            //if(POPCOUNT(*GRAPHROW1(nautyg, neighbour_b1, MAXM) & *GRAPHROW1(nautyg, neighbour_b2, MAXM)) >= 2)
            //  return 1; //contains diamond; n is a degree 2 vertex of the diamond
            //Nee, niet noodzakelijk induced!

            int neighbour_b2 = -1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b2)) >= 0) {
                //b1 b2 central edge
                setword intersect_neigh_b1_b2 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *GRAPHROW1(nautyg, neighbour_b2, MAXM);
                int popcnt2 = POPCOUNT(intersect_neigh_b1_b2);
                if(popcnt2 >= 2) {
                    //n-1 must be one end of the corner!!!
                    int neighbour_b1_b2 = -1;
                    while((neighbour_b1_b2 = nextelement1(&intersect_neigh_b1_b2, 1, neighbour_b1_b2)) >= 0) {
                        if(neighbour_b1_b2 != nv - 1) {
                            if(!ISELEMENT1(&nautyg[neighbour_b1_b2], nv - 1)) {
                                return 1; //Induced diamond
                            }
                        }
                    }
                }

            }

        }
    }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a triangle which also contains the last vertex, else returns 0
//The last vertex must be in the triangle, since the parents were triangle-free
//int n: nv of the graph
static int
contains_triangle() {
    set *gv = GRAPHROW1(nautyg, nv-1, MAXM);
    int neighbour_b1 = -1;
    while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
        setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
        if(intersect_neigh_n_b1 != 0)
            return 1;
    }

    return 0;
}

/******************************************************************************/

/**
 * Sort array into ascending order using bubblesort.
 */
//Could use mergesort or quicksort, but won't make a big difference since the arrays are very small
static void
sort_array_bubblesort(unsigned char vertexset[], int size) {
    int i, j, temp;
    for(j = 0; j < size; j++) {
        for(i = 1; i < size - j; i++) {
            if(vertexset[i - 1] > vertexset[i]) {
                temp = vertexset[i];
                vertexset[i] = vertexset[i - 1];
                vertexset[i - 1] = temp;
            }
        }
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
                if(!NBH_IS_SUBSET_OF(compare_array[i], current_array[i]))
                    break;
            if(i == array_size) {
                is_sim_pair = 1;
                for(i = 0; i < array_size; i++) {
                    best_pair[i] = compare_array[i];
                    best_pair[array_size + i] = current_array[i];
                }

                return;
            }

            for(i = 0; i < array_size; i++)
                if(!NBH_IS_SUBSET_OF(current_array[i], compare_array[i]))
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
            if(array_size == 2) {
                unsigned char removed_edge[2];
                removed_edge[0] = compare_array[0];
                removed_edge[1] = compare_array[1];
                sort_array_bubblesort(removed_edge, 2);

                for(i = 0; i < array_size; i++)
                    if(!NBH_IS_SUBSET_OF_COL_DOM_EDGE_REMOVED(compare_array[i], current_array[i],
                                                              removed_edge[0], removed_edge[1]))
                        break;
                if(i == array_size) {
                    is_sim_pair = 1;
                    for(i = 0; i < array_size; i++) {
                        best_pair[i] = compare_array[i];
                        best_pair[array_size + i] = current_array[i];
                    }

                    return;
                }

                removed_edge[0] = current_array[0];
                removed_edge[1] = current_array[1];
                sort_array_bubblesort(removed_edge, 2);

                for(i = 0; i < array_size; i++)
                    if(!NBH_IS_SUBSET_OF_COL_DOM_EDGE_REMOVED(current_array[i], compare_array[i],
                                                              removed_edge[0], removed_edge[1]))
                        break;
                if(i == array_size) {
                    is_sim_pair = 1;

                    for(i = 0; i < array_size; i++) {
                        best_pair[array_size + i] = compare_array[i];
                        best_pair[i] = current_array[i];
                    }

                    return;
                }
            } else if(array_size == 3) {
                unsigned char removed_triangle[3];
                for(i = 0; i < array_size; i++)
                    removed_triangle[i] = compare_array[i];
                sort_array_bubblesort(removed_triangle, 3);

                for(i = 0; i < array_size; i++)
                    if(!NBH_IS_SUBSET_OF_COL_DOM_TRIANGLE_REMOVED(compare_array[i], current_array[i],
                                                                  removed_triangle[0], removed_triangle[1], removed_triangle[2]))
                        break;
                if(i == array_size) {
                    is_sim_pair = 1;
                    for(i = 0; i < array_size; i++) {
                        best_pair[i] = compare_array[i];
                        best_pair[array_size + i] = current_array[i];
                    }

                    return;
                }

                for(i = 0; i < array_size; i++)
                    removed_triangle[i] = current_array[i];
                sort_array_bubblesort(removed_triangle, 3);

                for(i = 0; i < array_size; i++)
                    if(!NBH_IS_SUBSET_OF_COL_DOM_TRIANGLE_REMOVED(current_array[i], compare_array[i],
                                                                  removed_triangle[0], removed_triangle[1], removed_triangle[2]))
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
                fprintf(stderr, "Error: use_col_dom_graph for size > 3 not yet supported!\n");
                exit(1);
            }
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

//Tests if the pair is a similar pair.
//Important: the total size of possible_similar_pair is assumed to be 2*size_single
//Fixes the first x elements
static int
is_similar_pair_fix_first_elements(unsigned char possible_similar_pair[], int size_single,
                                   int use_col_dom_graph, int num_first_elements_to_fix) {
    if(num_first_elements_to_fix > size_single) {
        fprintf(stderr, "Error: fixed too many vertices!\n");
        exit(1);
    }

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

    //Fix first element
    for(i = 0; i < num_first_elements_to_fix; i++) {
        MARK(i);
        current_array[i] = array[i];
    }
    make_all_possible_combinations(array, size_single, current_array, num_first_elements_to_fix,
                                   compare_array, possible_similar_pair, use_col_dom_graph);

    return is_sim_pair;

}

/******************************************************************************/

static int
cycle_was_already_stored(setword cycle_bitvector) {
    int i;
    for(i = 0; i < num_cycles_stored; i++)
        if(stored_cycles_bitvectors[i] == cycle_bitvector)
            return 1;

    return 0;
}

/******************************************************************************/

/**
 * Returns 1 if the cubic cycle is a valid crown, else returns 0.
 *
 * All even cycles are valid, but only odd crowns where there is no colouring
 * such that the neighbours of the odd cycle all have the same colour are allowed!
 */
static int is_valid_crown(setword cycle_bitvector, int cycle_size) {
    if((cycle_size % 2) == 0)
        return 1;

    //Has to consider colourings of G - C
    make_graph_with_removed_vertices(cycle_bitvector);
    //printgraph_minibaum(current_graph_mb, adj_mb, nv);

    setword cycle_neighbourhood = 0;
    int i;
    for(i = 0; i < nv; i++)
        if(ISELEMENT1(&cycle_bitvector, i)) {
            cycle_neighbourhood |= nautyg[i];
        }
    //Cycle vertices also must receive the same colour since they are now isolated!
    //cycle_neighbourhood &= ~cycle_bitvector;

    //Ok, getest, klopt!
    //fprintf(stderr, "cycle neighbourhood: %u\n", cycle_neighbourhood);

    if(chromatic_number_same_colour_vertices(current_graph_mb, adj_mb, num_colours,
                                             cycle_neighbourhood) == 0) {
        //There is no valid colouring where the neighbours of the odd cycle all have the same colour
        //So this is a valid odd crown!
        //fprintf(stderr, "nv=%d, valid odd crown of size %d\n", nv, cycle_size);
        return 1;
    } else {
        //fprintf(stderr, "nv=%d, invalid odd crown of size %d\n", nv, cycle_size);
        return 0;
    }
}

/******************************************************************************/

//It is assumed that current_pathlength > 0
static void
search_forbidden_induced_cycles_recursion(int forbiddencyclesize) {
    if(current_pathsize >= forbiddencyclesize)
        return;

    //TODO: temp just store 1 cycle
    //if(num_cycles_stored > 0)
    //    return;

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
                search_forbidden_induced_cycles_recursion(forbiddencyclesize);

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

                current_path[current_pathsize++] = next_vertex;
                ADDELEMENT1(&current_path_bitvector, next_vertex);

                if(is_valid_crown(current_path_bitvector, forbiddencyclesize)
                   && !cycle_was_already_stored(current_path_bitvector)) {

                    //Store cycle
                    if(num_cycles_stored == MAX_NUM_CYCLES) {
                        fprintf(stderr, "Error: MAX_NUM_CYCLES is not high enough!\n");
                        exit(1);
                    }

                    memcpy(stored_cycles[num_cycles_stored], current_path, sizeof (int) * current_pathsize);
                    stored_cycles_bitvectors[num_cycles_stored] = current_path_bitvector;

/*
                    fprintf(stderr, "found induced C%d: \n", current_pathsize);
                    int k;
                    for(k = 0; k < current_pathsize; k++)
                        fprintf(stderr, "%d ", current_path[k]);
                    fprintf(stderr, "\n");
*/

                    num_cycles_stored++;
                }

                //Restore
                current_pathsize--;
                DELELEMENT1(&current_path_bitvector, next_vertex);
            }
        }
    }
}

/******************************************************************************/

static void
search_forbidden_induced_cycles(setword possible_vertices, int forbiddencyclesize) {
    //possible_cycle_vertices = ALLMASK(nv);
    //possible_cycle_vertices = ALLMASK(nv - 1);
    possible_cycle_vertices = possible_vertices;

    num_cycles_stored = 0;

    current_pathsize = 1;

    int i;
    for(i = 0; i < nv; i++) {
        if(ISELEMENT1(&possible_cycle_vertices, i)) {
            current_path[0] = i;

            current_path_bitvector = 0;
            ADDELEMENT1(&current_path_bitvector, i);
            DELELEMENT1(&possible_cycle_vertices, i);
            search_forbidden_induced_cycles_recursion(forbiddencyclesize);

            //TODO: temp just store 1 cycle
            //if(num_cycles_stored > 0)
            //    return;

            //Restore
            ADDELEMENT1(&possible_cycle_vertices, i);
        }
    }

/*
    if(num_cycles_stored > 0) {
        fprintf(stderr, "Num cycles stored: %d\n", num_cycles_stored);
        fprintf(stderr, "graph was:\n");
        printgraph(current_graph, adj, nv);
        fprintf(stderr, "\n");
    }
*/

}

/******************************************************************************/

/**
 * Returns 1 if the graph contains an even crown (i.e. an even cubic cycle).
 */
//TODO: probably even doesn't have to be an induced cycle!
static int contains_even_crown(setword *crown_vertices) {
    //Mark cubic vertices
    setword cubic_vertices = 0;
    int i;
    for(i = 0; i < nv; i++)
        if(adj[i] == 3)
            ADDELEMENT1(&cubic_vertices, i);

    //fprintf(stderr, "Num cubic vertices: %d\n", POPCOUNT(cubic_vertices));
    //printgraph(current_graph, adj, nv);

    //Search for forbidden induced cycles
    //If Pt-free: longest induced cycle max Ct (i.e. max_path_size + 1)
    int start = 4;
    if(test_girth && girth > start) {
        if((girth % 2) == 0)
            start = girth;
        else
            start = girth + 1;
    }

    //fprintf(stderr, "start: %d\n", start);

    for(i = start; i <= max_path_size + 1; i += 2)
        if(!(test_if_contains_forbidden_cycle && forbidden_cycle_size == i)) {

            //TODO: als meerdere crowns, welke is de beste?
            //Bv buren grootste graad?
            //Of alle proberen en die met minste aantal kinderen nemen?

            search_forbidden_induced_cycles(cubic_vertices, i);

            //fprintf(stderr, "Nv=%d, num crowns of size %d: %d\n", nv, i, num_cycles_stored);

            if(num_cycles_stored > 0) {
                *crown_vertices = stored_cycles_bitvectors[0];
                return 1;
            }
        }

    return 0;

}

/******************************************************************************/

/**
 * Returns 1 if the graph contains an odd crown (i.e. an even cubic cycle).
 *
 * Important: not any odd crown is allowed! Only odd crowns where there is no colouring
 * such that the neighbours of the odd cycle all have the same colour are allowed!
 */
//TODO: probably even doesn't have to be an induced cycle!
static int contains_odd_crown(setword *crown_vertices) {
    //Mark cubic vertices
    setword cubic_vertices = 0;
    int i;
    for(i = 0; i < nv; i++)
        if(adj[i] == 3)
            ADDELEMENT1(&cubic_vertices, i);

    //fprintf(stderr, "Num cubic vertices: %d\n", POPCOUNT(cubic_vertices));
    //printgraph(current_graph, adj, nv);

    //Search for forbidden induced cycles
    //If Pt-free: longest induced cycle max Ct (i.e. max_path_size + 1)
    int start = 3;
    if(test_if_trianglefree)
        start = 5;
    if(test_girth && girth > start) {
        if((girth % 2) == 0)
            start = girth + 1;
        else
            start = girth;
    }

    //fprintf(stderr, "start: %d\n", start);

    for(i = start; i <= max_path_size + 1; i += 2)
        if(!(test_if_contains_forbidden_cycle && forbidden_cycle_size == i)) {

            //TODO: als meerdere crowns, welke is de beste?
            //Bv buren grootste graad?
            //Of alle proberen en die met minste aantal kinderen nemen?

            search_forbidden_induced_cycles(cubic_vertices, i);

            //fprintf(stderr, "Nv=%d, num crowns of size %d: %d\n", nv, i, num_cycles_stored);

            if(num_cycles_stored > 0) {
                *crown_vertices = stored_cycles_bitvectors[0];
                return 1;
            }
        }

    return 0;

}

/******************************************************************************/

//Returns 1 if the graph contains a disjoint similar diamond pair
//else returns 0.
//If 1 is returned, best_similar_diamondpair points to the *best* similar diamond pair which should be destroyed in the next step
//best_similar_diamondpair[0] = a1
//best_similar_diamondpair[1] = a2
//best_similar_diamondpair[2] = a3
//best_similar_diamondpair[3] = a4
//best_similar_diamondpair[4] = b1
//best_similar_diamondpair[5] = b2
//best_similar_diamondpair[6] = b3
//best_similar_diamondpair[7] = b4
//Important: it is assumed that the graph is K4-free
static int contains_similar_diamond(unsigned char best_similar_diamondpair[8], int use_col_dom_graph) {
    if(num_colours > 3) {
        //Return here since we assume the graph contains no K4's
        return 0;
    }

    if(use_col_dom_graph) {
        //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
        fprintf(stderr, "Error: colour domination not supported for similar P4's!\n");
        exit(1);
    }

    //Search for all possible disjoint triangles
    int contains_sim_diamondpair = 0;

    //Remark: possibly there are more efficient methods, but this is not a bottleneck!

    int a, b, c, d, e, f, g, h;
    int j, k;
    //diamonds: a b c d and e f g h
    //a < d and b < c
    //a < e and e < h and f < g
    setword neighbourhood_a_b, neighbourhood_b_c;
    setword neighbourhood_e_f, neighbourhood_f_g;
    for(a = 0; a < nv; a++)
        for(j = 0; j < adj[a]; j++) {
            b = current_graph[a][j];
            neighbourhood_a_b = *GRAPHROW1(nautyg, a, MAXM) & *GRAPHROW1(nautyg, b, MAXM);
            c = b; //since we want that c > b
            while((c = nextelement1(&neighbourhood_a_b, 1, c)) >= 0) {
                neighbourhood_b_c = *GRAPHROW1(nautyg, b, MAXM) & *GRAPHROW1(nautyg, c, MAXM);
                d = a; //since we want that d > a
                while((d = nextelement1(&neighbourhood_b_c, 1, d)) >= 0) {
                    //diamond a b c d formed
                    best_similar_diamondpair[0] = a;
                    best_similar_diamondpair[1] = d;
                    best_similar_diamondpair[2] = b;
                    best_similar_diamondpair[3] = c;

                    setword forbidden_vertices = 0;
                    ADDELEMENT1(&forbidden_vertices, a);
                    ADDELEMENT1(&forbidden_vertices, b);
                    ADDELEMENT1(&forbidden_vertices, c);
                    ADDELEMENT1(&forbidden_vertices, d);

                    for(e = a + 1; e < nv; e++) {
                        if(!ISELEMENT1(&forbidden_vertices, e))
                            for(k = 0; k < adj[e]; k++) {
                                f = current_graph[e][k];
                                if(!ISELEMENT1(&forbidden_vertices, f)) {
                                    neighbourhood_e_f = *GRAPHROW1(nautyg, e, MAXM) & *GRAPHROW1(nautyg, f, MAXM);
                                    g = f; //since we want that g > f
                                    while((g = nextelement1(&neighbourhood_e_f, 1, g)) >= 0) {
                                        if(!ISELEMENT1(&forbidden_vertices, g)) {
                                            neighbourhood_f_g = *GRAPHROW1(nautyg, f, MAXM) & *GRAPHROW1(nautyg, g, MAXM);
                                            h = e; //since we want that h > e
                                            while((h = nextelement1(&neighbourhood_f_g, 1, h)) >= 0) {
                                                if(!ISELEMENT1(&forbidden_vertices, h)) {
                                                    //Disjoint diamond e f g h formed
                                                    best_similar_diamondpair[4] = e;
                                                    best_similar_diamondpair[5] = h;
                                                    best_similar_diamondpair[6] = f;
                                                    best_similar_diamondpair[7] = g;

                                                    DELELEMENT1(&nautyg[a], b);
                                                    DELELEMENT1(&nautyg[a], c);
                                                    DELELEMENT1(&nautyg[b], a);
                                                    DELELEMENT1(&nautyg[b], c);
                                                    DELELEMENT1(&nautyg[b], d);
                                                    DELELEMENT1(&nautyg[d], b);
                                                    DELELEMENT1(&nautyg[d], c);
                                                    DELELEMENT1(&nautyg[c], a);
                                                    DELELEMENT1(&nautyg[c], b);
                                                    DELELEMENT1(&nautyg[c], d);

                                                    DELELEMENT1(&nautyg[e], f);
                                                    DELELEMENT1(&nautyg[e], g);
                                                    DELELEMENT1(&nautyg[f], e);
                                                    DELELEMENT1(&nautyg[f], h);
                                                    DELELEMENT1(&nautyg[f], g);
                                                    DELELEMENT1(&nautyg[h], f);
                                                    DELELEMENT1(&nautyg[h], g);
                                                    DELELEMENT1(&nautyg[g], e);
                                                    DELELEMENT1(&nautyg[g], f);
                                                    DELELEMENT1(&nautyg[g], h);

                                                    //First first 2 elements
                                                    contains_sim_diamondpair = is_similar_pair_fix_first_elements(best_similar_diamondpair,
                                                                                                                  4, use_col_dom_graph, 2);

                                                    //Now also try other combination (if no sim pair):
                                                    if(!contains_sim_diamondpair) {
                                                        best_similar_diamondpair[4] = h;
                                                        best_similar_diamondpair[5] = e;

                                                        contains_sim_diamondpair = is_similar_pair_fix_first_elements(best_similar_diamondpair,
                                                                                                                      4, use_col_dom_graph, 2);
                                                    }

                                                    //Restore neighbourhoods
                                                    ADDELEMENT1(&nautyg[a], b);
                                                    ADDELEMENT1(&nautyg[a], c);
                                                    ADDELEMENT1(&nautyg[b], a);
                                                    ADDELEMENT1(&nautyg[b], c);
                                                    ADDELEMENT1(&nautyg[b], d);
                                                    ADDELEMENT1(&nautyg[d], b);
                                                    ADDELEMENT1(&nautyg[d], c);
                                                    ADDELEMENT1(&nautyg[c], a);
                                                    ADDELEMENT1(&nautyg[c], b);
                                                    ADDELEMENT1(&nautyg[c], d);

                                                    ADDELEMENT1(&nautyg[e], f);
                                                    ADDELEMENT1(&nautyg[e], g);
                                                    ADDELEMENT1(&nautyg[f], e);
                                                    ADDELEMENT1(&nautyg[f], h);
                                                    ADDELEMENT1(&nautyg[f], g);
                                                    ADDELEMENT1(&nautyg[h], f);
                                                    ADDELEMENT1(&nautyg[h], g);
                                                    ADDELEMENT1(&nautyg[g], e);
                                                    ADDELEMENT1(&nautyg[g], f);
                                                    ADDELEMENT1(&nautyg[g], h);

                                                    if(contains_sim_diamondpair)
                                                        return 1;

                                                }
                                            }
                                        }
                                    }
                                }
                            }
                    }

                }
            }
        }


    return 0;
}

/******************************************************************************/

static int contains_similar_P6(unsigned char best_similar_P6pair[12], int use_col_dom_graph) {
    //Search for all possible disjoint triangles
    int contains_sim_P6pair = 0;

    //TODO: possibly there are more efficient methods, but this is probably not a bottleneck!

    if(use_col_dom_graph) {
        //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
        fprintf(stderr, "Error: colour domination not supported for similar P6's!\n");
        exit(1);
    }

    int a, b, c, d, a1, a2;
    int e, f, g, h, e1, e2;
    int j, k, i, l, m, n, o, p, q, r;

    setword forbidden_nbrs1, forbidden_nbrs2;

    //Only demanding a < d
    //and e < h and a < e
    setword neighbourhood_b_c, neighbourhood_f_g;
    for(a = 0; a < nv; a++)
        for(j = 0; j < adj[a]; j++) {
            b = current_graph[a][j];
            for(k = 0; k < adj[b]; k++) {
                c = current_graph[b][k];
                //a and c are no neighbours
                if(c != a && !ISELEMENT1(&nautyg[a], c)) {
                    for(i = 0; i < adj[c]; i++) {
                        d = current_graph[c][i];
                        if(d != b && !ISELEMENT1(&nautyg[b], d)
                           && !ISELEMENT1(&nautyg[a], d)) {

                            forbidden_nbrs1 = 0;
                            ADDELEMENT1(&forbidden_nbrs1, a);
                            ADDELEMENT1(&forbidden_nbrs1, b);
                            ADDELEMENT1(&forbidden_nbrs1, c);
                            for(o = 0; o < adj[d]; o++) {
                                a1 = current_graph[d][o];
                                if((nautyg[a1] & forbidden_nbrs1) == 0) {
                                    //Also automatically implies a1 != c
                                    ADDELEMENT1(&forbidden_nbrs1, d);
                                    for(p = 0; p < adj[a1]; p++) {
                                        a2 = current_graph[a1][p];
                                        if(a < a2 && (nautyg[a2] & forbidden_nbrs1) == 0) {
                                            //Also automatically implies a2 != d

                                            //Induced P6 found
                                            best_similar_P6pair[0] = a;
                                            best_similar_P6pair[1] = b;
                                            best_similar_P6pair[2] = c;
                                            best_similar_P6pair[3] = d;
                                            best_similar_P6pair[4] = a1;
                                            best_similar_P6pair[5] = a2;

                                            setword forbidden_vertices = 0;
                                            ADDELEMENT1(&forbidden_vertices, a);
                                            ADDELEMENT1(&forbidden_vertices, b);
                                            ADDELEMENT1(&forbidden_vertices, c);
                                            ADDELEMENT1(&forbidden_vertices, d);
                                            ADDELEMENT1(&forbidden_vertices, a1);
                                            ADDELEMENT1(&forbidden_vertices, a2);

                                            //e < e2 and a < e
                                            for(e = a + 1; e < nv; e++)
                                                if(!ISELEMENT1(&forbidden_vertices, e))
                                                    for(l = 0; l < adj[e]; l++) {
                                                        f = current_graph[e][l];
                                                        if(!ISELEMENT1(&forbidden_vertices, f))
                                                            for(m = 0; m < adj[f]; m++) {
                                                                g = current_graph[f][m];

                                                                //e and g are no neighbours
                                                                if(g != e && !ISELEMENT1(&nautyg[g], e)
                                                                   && !ISELEMENT1(&forbidden_vertices, g)) {

                                                                    for(n = 0; n < adj[g]; n++) {
                                                                        h = current_graph[g][n];
                                                                        if(h != f && !ISELEMENT1(&nautyg[f], h)
                                                                           && !ISELEMENT1(&nautyg[e], h) && !ISELEMENT1(&forbidden_vertices, h)) {
                                                                            forbidden_nbrs2 = 0;
                                                                            ADDELEMENT1(&forbidden_nbrs2, e);
                                                                            ADDELEMENT1(&forbidden_nbrs2, f);
                                                                            ADDELEMENT1(&forbidden_nbrs2, g);

                                                                            for(q = 0; q < adj[h]; q++) {
                                                                                e1 = current_graph[h][q];
                                                                                if(!ISELEMENT1(&forbidden_vertices, e1)
                                                                                   && (nautyg[e1] & forbidden_nbrs2) == 0) {
                                                                                    //Also automatically implies e1 != g
                                                                                    ADDELEMENT1(&forbidden_nbrs2, h);

                                                                                    for(r = 0; r < adj[e1]; r++) {
                                                                                        e2 = current_graph[e1][r];
                                                                                        if(e < e2 && !ISELEMENT1(&forbidden_vertices, e2)
                                                                                           && (nautyg[e2] & forbidden_nbrs2) == 0) {
                                                                                            //Also automatically implies e2 != h

                                                                                            //Induced P6 found
                                                                                            best_similar_P6pair[6] = e;
                                                                                            best_similar_P6pair[7] = f;
                                                                                            best_similar_P6pair[8] = g;
                                                                                            best_similar_P6pair[9] = h;
                                                                                            best_similar_P6pair[10] = e1;
                                                                                            best_similar_P6pair[11] = e2;


                                                                                            //TODO: maybe remove earlier if more efficient?
                                                                                            //But is probably no bottleneck?
                                                                                            //Remove vertices from edges from neighbourhoods
                                                                                            DELELEMENT1(&nautyg[a], b);
                                                                                            DELELEMENT1(&nautyg[b], a);
                                                                                            DELELEMENT1(&nautyg[b], c);
                                                                                            DELELEMENT1(&nautyg[c], b);
                                                                                            DELELEMENT1(&nautyg[c], d);
                                                                                            DELELEMENT1(&nautyg[d], c);

                                                                                            DELELEMENT1(&nautyg[d], a1);
                                                                                            DELELEMENT1(&nautyg[a1], d);
                                                                                            DELELEMENT1(&nautyg[a1], a2);
                                                                                            DELELEMENT1(&nautyg[a2], a1);

                                                                                            DELELEMENT1(&nautyg[e], f);
                                                                                            DELELEMENT1(&nautyg[f], e);
                                                                                            DELELEMENT1(&nautyg[f], g);
                                                                                            DELELEMENT1(&nautyg[g], f);
                                                                                            DELELEMENT1(&nautyg[g], h);
                                                                                            DELELEMENT1(&nautyg[h], g);

                                                                                            DELELEMENT1(&nautyg[h], e1);
                                                                                            DELELEMENT1(&nautyg[e1], h);
                                                                                            DELELEMENT1(&nautyg[e1], e2);
                                                                                            DELELEMENT1(&nautyg[e2], e1);

                                                                                            //Now test...
                                                                                            contains_sim_P6pair = is_similar_pair_fix_first_elements(best_similar_P6pair,
                                                                                                                                                     6, use_col_dom_graph, 6);


                                                                                            //Now also try other combination (if no sim pair):
                                                                                            if(!contains_sim_P6pair) {
                                                                                                //Only other possibility:
                                                                                                best_similar_P6pair[6] = e2;
                                                                                                best_similar_P6pair[7] = e1;
                                                                                                best_similar_P6pair[8] = h;
                                                                                                best_similar_P6pair[9] = g;
                                                                                                best_similar_P6pair[10] = f;
                                                                                                best_similar_P6pair[11] = e;

                                                                                                contains_sim_P6pair = is_similar_pair_fix_first_elements(best_similar_P6pair,
                                                                                                                                                         6, use_col_dom_graph, 6);
                                                                                            }

                                                                                            //Restore neighbourhoods
                                                                                            ADDELEMENT1(&nautyg[a], b);
                                                                                            ADDELEMENT1(&nautyg[b], a);
                                                                                            ADDELEMENT1(&nautyg[b], c);
                                                                                            ADDELEMENT1(&nautyg[c], b);
                                                                                            ADDELEMENT1(&nautyg[c], d);
                                                                                            ADDELEMENT1(&nautyg[d], c);

                                                                                            ADDELEMENT1(&nautyg[d], a1);
                                                                                            ADDELEMENT1(&nautyg[a1], d);
                                                                                            ADDELEMENT1(&nautyg[a1], a2);
                                                                                            ADDELEMENT1(&nautyg[a2], a1);

                                                                                            ADDELEMENT1(&nautyg[e], f);
                                                                                            ADDELEMENT1(&nautyg[f], e);
                                                                                            ADDELEMENT1(&nautyg[f], g);
                                                                                            ADDELEMENT1(&nautyg[g], f);
                                                                                            ADDELEMENT1(&nautyg[g], h);
                                                                                            ADDELEMENT1(&nautyg[h], g);

                                                                                            ADDELEMENT1(&nautyg[h], e1);
                                                                                            ADDELEMENT1(&nautyg[e1], h);
                                                                                            ADDELEMENT1(&nautyg[e1], e2);
                                                                                            ADDELEMENT1(&nautyg[e2], e1);

                                                                                            if(contains_sim_P6pair)
                                                                                                return 1;
                                                                                        }
                                                                                    }

                                                                                    DELELEMENT1(&forbidden_nbrs2, h);

                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }

                                                            }
                                                    }
                                        }
                                    }
                                    DELELEMENT1(&forbidden_nbrs1, d);

                                }
                            }
                        }
                    }
                }
            }
        }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a disjoint similar edgepair (see Sawada et al. Lemma 3.1 case m=2)
//else returns 0.
//If 1 is returned, best_similar_edgepair points to the *best* similar vertex which should be destroyed in the next step
//best_similar_P4pair[0] = a1
//best_similar_P4pair[1] = a2
//best_similar_P4pair[2] = a3
//best_similar_P4pair[3] = a4
//best_similar_P4pair[4] = b1
//best_similar_P4pair[5] = b2
//best_similar_P4pair[6] = b3
//best_similar_P4pair[7] = b4
static int contains_similar_P4(unsigned char best_similar_P4pair[8], int use_col_dom_graph) {
    //Search for all possible disjoint triangles
    int contains_sim_P4pair = 0;

    //TODO: possibly there are more efficient methods, but this is probably not a bottleneck!

    if(use_col_dom_graph) {
        //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
        fprintf(stderr, "Error: colour domination not supported for similar P4's!\n");
        exit(1);
    }

    int a, b, c, d;
    int e, f, g, h;
    int j, k, i, l, m, n;

    //Only demanding a < d
    //and e < h and a < e
    setword neighbourhood_b_c, neighbourhood_f_g;
    for(a = 0; a < nv; a++)
        for(j = 0; j < adj[a]; j++) {
            b = current_graph[a][j];
            for(k = 0; k < adj[b]; k++) {
                c = current_graph[b][k];
                //a and c are no neighbours
                if(c != a && !ISELEMENT1(&nautyg[a], c)) {
                    for(i = 0; i < adj[c]; i++) {
                        d = current_graph[c][i];
                        if(d != b && a < d && !ISELEMENT1(&nautyg[b], d)
                           && !ISELEMENT1(&nautyg[a], d)) {
                            //Induced P4 found
                            best_similar_P4pair[0] = a;
                            best_similar_P4pair[1] = b;
                            best_similar_P4pair[2] = c;
                            best_similar_P4pair[3] = d;

                            setword forbidden_vertices = 0;
                            ADDELEMENT1(&forbidden_vertices, a);
                            ADDELEMENT1(&forbidden_vertices, b);
                            ADDELEMENT1(&forbidden_vertices, c);
                            ADDELEMENT1(&forbidden_vertices, d);

                            //e < h and a < e
                            for(e = a + 1; e < nv; e++)
                                if(!ISELEMENT1(&forbidden_vertices, e))
                                    for(l = 0; l < adj[e]; l++) {
                                        f = current_graph[e][l];
                                        if(!ISELEMENT1(&forbidden_vertices, f))
                                            for(m = 0; m < adj[f]; m++) {
                                                g = current_graph[f][m];

                                                //e and g are no neighbours
                                                if(g != e && !ISELEMENT1(&nautyg[g], e)
                                                   && !ISELEMENT1(&forbidden_vertices, g)) {

                                                    for(n = 0; n < adj[g]; n++) {
                                                        h = current_graph[g][n];
                                                        if(h != f && e < h && !ISELEMENT1(&nautyg[f], h)
                                                           && !ISELEMENT1(&nautyg[e], h) && !ISELEMENT1(&forbidden_vertices, h)) {
                                                            //Induced C4 found
                                                            best_similar_P4pair[4] = e;
                                                            best_similar_P4pair[5] = f;
                                                            best_similar_P4pair[6] = g;
                                                            best_similar_P4pair[7] = h;


                                                            //TODO: maybe remove earlier if more efficient?
                                                            //But is probably no bottleneck?
                                                            //Remove vertices from edges from neighbourhoods
                                                            DELELEMENT1(&nautyg[a], b);
                                                            DELELEMENT1(&nautyg[b], a);
                                                            DELELEMENT1(&nautyg[b], c);
                                                            DELELEMENT1(&nautyg[c], b);
                                                            DELELEMENT1(&nautyg[c], d);
                                                            DELELEMENT1(&nautyg[d], c);

                                                            DELELEMENT1(&nautyg[e], f);
                                                            DELELEMENT1(&nautyg[f], e);
                                                            DELELEMENT1(&nautyg[f], g);
                                                            DELELEMENT1(&nautyg[g], f);
                                                            DELELEMENT1(&nautyg[g], h);
                                                            DELELEMENT1(&nautyg[h], g);

                                                            //Now test...
                                                            contains_sim_P4pair = is_similar_pair_fix_first_elements(best_similar_P4pair,
                                                                                                                     4, use_col_dom_graph, 4);


                                                            //Now also try other combination (if no sim pair):
                                                            if(!contains_sim_P4pair) {
                                                                //Only other possibility:
                                                                best_similar_P4pair[4] = h;
                                                                best_similar_P4pair[5] = g;
                                                                best_similar_P4pair[6] = f;
                                                                best_similar_P4pair[7] = e;

                                                                contains_sim_P4pair = is_similar_pair_fix_first_elements(best_similar_P4pair,
                                                                                                                         4, use_col_dom_graph, 4);
                                                            }

                                                            //Restore neighbourhoods
                                                            ADDELEMENT1(&nautyg[a], b);
                                                            ADDELEMENT1(&nautyg[b], a);
                                                            ADDELEMENT1(&nautyg[b], c);
                                                            ADDELEMENT1(&nautyg[c], b);
                                                            ADDELEMENT1(&nautyg[c], d);
                                                            ADDELEMENT1(&nautyg[d], c);

                                                            ADDELEMENT1(&nautyg[e], f);
                                                            ADDELEMENT1(&nautyg[f], e);
                                                            ADDELEMENT1(&nautyg[f], g);
                                                            ADDELEMENT1(&nautyg[g], f);
                                                            ADDELEMENT1(&nautyg[g], h);
                                                            ADDELEMENT1(&nautyg[h], g);

                                                            if(contains_sim_P4pair)
                                                                return 1;

                                                        }
                                                    }
                                                }

                                            }
                                    }
                        }
                    }
                }
            }
        }

    return 0;
}

/******************************************************************************/

//Returns 1 if the graph contains a disjoint similar edgepair (see Sawada et al. Lemma 3.1 case m=2)
//else returns 0.
//If 1 is returned, best_similar_edgepair points to the *best* similar vertex which should be destroyed in the next step
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
static int contains_similar_triangle(unsigned char best_similar_trianglepair[6], int use_col_dom_graph) {
    //Search for all possible disjoint triangles
    int contains_sim_trianglepair = 0;

    //Possibly there are more efficient methods, but this is probably not a bottleneck!

    int a, b, c, d, e, f;
    int j, k;
    // a < b < c and a < d < e < f
    setword neighbourhood_a_b, neighbourhood_d_e;
    for(a = 0; a < nv; a++) {
        for(j = 0; j < adj[a]; j++) {
            b = current_graph[a][j];
            if(a < b) {
                neighbourhood_a_b = *GRAPHROW1(nautyg, a, MAXM) & *GRAPHROW1(nautyg, b, MAXM);
                c = b; //since we want that c > b
                while((c = nextelement1(&neighbourhood_a_b, 1, c)) >= 0) {
                    //triangle a < b < c formed
                    best_similar_trianglepair[0] = a;
                    best_similar_trianglepair[1] = b;
                    best_similar_trianglepair[2] = c;
                    for(d = a + 1; d < nv; d++)
                        if(d != b && d != c) //d != a is already implied
                            for(k = 0; k < adj[d]; k++) {
                                e = current_graph[d][k];
                                if(d < e && e != b && e != c) {
                                    neighbourhood_d_e = *GRAPHROW1(nautyg, d, MAXM) & *GRAPHROW1(nautyg, e, MAXM);
                                    f = e;
                                    while((f = nextelement1(&neighbourhood_d_e, 1, f)) >= 0) {
                                        if(f != b && f != c) { //f > e is already implied
                                            //disjoint triangle d e f formed
                                            //fprintf(stderr, "disjoint triangles: (%d %d %d) and (%d %d %d)\n",a,b,c,d,e,f);

                                            DELELEMENT1(&nautyg[a], b);
                                            DELELEMENT1(&nautyg[a], c);
                                            DELELEMENT1(&nautyg[b], a);
                                            DELELEMENT1(&nautyg[b], c);
                                            DELELEMENT1(&nautyg[c], a);
                                            DELELEMENT1(&nautyg[c], b);

                                            DELELEMENT1(&nautyg[d], e);
                                            DELELEMENT1(&nautyg[d], f);
                                            DELELEMENT1(&nautyg[e], d);
                                            DELELEMENT1(&nautyg[e], f);
                                            DELELEMENT1(&nautyg[f], d);
                                            DELELEMENT1(&nautyg[f], e);

                                            //best_similar_trianglepair[0] = a;
                                            //best_similar_trianglepair[1] = b;
                                            //best_similar_trianglepair[2] = c;
                                            best_similar_trianglepair[3] = d;
                                            best_similar_trianglepair[4] = e;
                                            best_similar_trianglepair[5] = f;

                                            //Now test...
                                            contains_sim_trianglepair = is_similar_pair(best_similar_trianglepair,
                                                                                        3, use_col_dom_graph);

                                            //Restore neighbourhoods
                                            ADDELEMENT1(&nautyg[a], b);
                                            ADDELEMENT1(&nautyg[a], c);
                                            ADDELEMENT1(&nautyg[b], a);
                                            ADDELEMENT1(&nautyg[b], c);
                                            ADDELEMENT1(&nautyg[c], a);
                                            ADDELEMENT1(&nautyg[c], b);

                                            ADDELEMENT1(&nautyg[d], e);
                                            ADDELEMENT1(&nautyg[d], f);
                                            ADDELEMENT1(&nautyg[e], d);
                                            ADDELEMENT1(&nautyg[e], f);
                                            ADDELEMENT1(&nautyg[f], d);
                                            ADDELEMENT1(&nautyg[f], e);

                                            if(contains_sim_trianglepair)
                                                return 1;
                                        }
                                    }

                                }
                            }
                }
            }

        }

    }


    return 0;
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
    num_similar_edges = 0;

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
                                //Ok, verified that gives same result as ifs below
                                contains_sim_edgepair = is_similar_pair(best_similar_edgepair,
                                                                        2, use_col_dom_graph);

/*
                                //Test if (N(i) \seteq N(k)) and (N(next) \seteq N(next0)) etc.
                                if(NBH_IS_SUBSET_OF(i, k) && NBH_IS_SUBSET_OF(next, next0)) {
                                    best_similar_edgepair[0] = i;
                                    best_similar_edgepair[1] = next;
                                    best_similar_edgepair[2] = k;
                                    best_similar_edgepair[3] = next0;
                                    contains_sim_edgepair = 1;
                                } else if(NBH_IS_SUBSET_OF(k, i) && NBH_IS_SUBSET_OF(next0, next)) {
                                    best_similar_edgepair[0] = k;
                                    best_similar_edgepair[1] = next0;
                                    best_similar_edgepair[2] = i;
                                    best_similar_edgepair[3] = next;
                                    contains_sim_edgepair = 1;
                                } else if(NBH_IS_SUBSET_OF(i, next0) && NBH_IS_SUBSET_OF(next, k)) {
                                    best_similar_edgepair[0] = i;
                                    best_similar_edgepair[1] = next;
                                    best_similar_edgepair[2] = next0;
                                    best_similar_edgepair[3] = k;
                                    contains_sim_edgepair = 1;
                                } else if(NBH_IS_SUBSET_OF(next0, i) && NBH_IS_SUBSET_OF(k, next)) {
                                    best_similar_edgepair[0] = next0;
                                    best_similar_edgepair[1] = k;
                                    best_similar_edgepair[2] = i;
                                    best_similar_edgepair[3] = next;
                                    contains_sim_edgepair = 1;
                                }
*/

                                //Restore neighbourhoods
                                ADDELEMENT1(&nautyg[i], next);
                                ADDELEMENT1(&nautyg[next], i);
                                ADDELEMENT1(&nautyg[k], next0);
                                ADDELEMENT1(&nautyg[next0], k);

                                if(contains_sim_edgepair) {
                                    if(num_similar_edges == MAX_SIM_EDGES) {
                                        fprintf(stderr, "Error: too much sim edges!\n");
                                        exit(1);
                                    }

                                    similar_edges[num_similar_edges][0] = best_similar_edgepair[0];
                                    similar_edges[num_similar_edges][1] = best_similar_edgepair[1];
                                    similar_edges[num_similar_edges][2] = best_similar_edgepair[2];
                                    similar_edges[num_similar_edges][3] = best_similar_edgepair[3];
                                    num_similar_edges++;

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

//Returns 1 if the graph contains similar vertices (i.e. if there are 2 vertices u,v for which N(u) \seteq N(v))
//else returns 0.
//If 1 is returned, best_small_similar_vertex points to the *best* similar vertex which should be destroyed in the next step
//and other_larger_similar_vertex is the vertex which has a larger (or same) neighbourhood than best_small_similar_vertex
//The best similar vertex is the one with the largest degree (is significantly faster than taking a random one)
static int contains_similar_vertices(unsigned char *best_small_similar_vertex, unsigned char *other_larger_similar_vertex) {
    //unsigned char degree_current_best_small = 0; //Only connected graphs, so min deg will be > 0
    unsigned char degree_current_best_small = MAXN; //Min or max deg doesnt seem to make much difference here
    unsigned char degree_current_best_large = 0;
    //Min degree seems to be slightly better
    unsigned char current_best_small;
    unsigned char current_best_large;

    num_similar_vertices = 0;

    int i, j;
    for(i = 0; i < nv; i++)
        for(j = 0; j < nv; j++)
            if(i != j && NBH_IS_SUBSET_OF(i, j)) {
                //neighbourhood of i is contained in neighbourhood of j

                if(num_similar_vertices == MAX_SIM_VERTICES) {
                    fprintf(stderr, "Error: too much sim vertices!\n");
                    exit(1);
                }

                similar_vertices[num_similar_vertices][0] = i;
                similar_vertices[num_similar_vertices][1] = j;
                num_similar_vertices++;

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

    num_similar_vertices = 0;

    int i, j;
    for(i = 0; i < nv; i++)
        for(j = 0; j < nv; j++)
            if(i != j && NBH_IS_SUBSET_OF_COL_DOM_VERTEX_REMOVED(i, j)) {
                //neighbourhood of i is contained in neighbourhood of j

                if(num_similar_vertices == MAX_SIM_VERTICES) {
                    fprintf(stderr, "Error: too much sim vertices!\n");
                    exit(1);
                }

                similar_vertices[num_similar_vertices][0] = i;
                similar_vertices[num_similar_vertices][1] = j;
                num_similar_vertices++;

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

//(k+1)-critical graphs must have min degree >= k
//Returns 1 if the graphs contains a vertex with degree < k (= num_colours), else returns 0
//If 1 is returned, best_small_vertex points to the *best* small vertex
//It doesn't matter if the first one or the smallest or largest one is taken
//Girth case: seems best always to take small vertex with largest label!
static int contains_too_small_vertex(unsigned char *best_small_vertex) {
    int i;
    int current_best = -1;
    num_small_vertices = 0;
    for(i = 0; i < nv; i++)
        if(adj[i] < num_colours) {
            current_best = i;
            small_vertices[num_small_vertices++] = i;
        }
    //Helps a little bit if returning a vertex which is part of a triangle (in the diamondfree case)
    if(current_best != -1) {
        *best_small_vertex = current_best;
        return 1;
    } else
        return 0;
}

/**************************************************************************/

static void
add_edge_one_way(GRAPH graph, unsigned char adj[], unsigned char v, unsigned char w) {
    graph[v][adj[v]] = w;
    //graph[w][adj[w]] = v;
    adj[v]++;
    //adj[w]++;
}

/******************************************************************************/

//Returns 1 if the graph is vertex critical (i.e. all of its children is k-colourable)
//else returns 0
//Important 1: it is assumed that the graph is NOT k-colourable
//Important 2: we don't need to remove the last vertex since we know the parent is k-colourable
static int
is_vertex_critical() {
    //Remark: this method is not really a bottleneck

    //If the graph has a vertex of degree less than k, it cannot be (k+1)-critical
    //Here k == num_colours
    int i;
    for(i = 0; i < nv; i++)
        if(adj[i] < num_colours) {
            //fprintf(stderr, "reject degree!\n");
            return 0;
        }

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

        if(chromatic_number(current_graph_mb, adj_mb, num_colours) == 0) {
            //i.e. graph is not k-colourable, so the larger graph was not critical!
            return 0;
        }
    }

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

static void remove_edges_in_all_possible_ways(int edge_index, int num_edges_removed) {

    if(!is_critical)
        return;

    if(num_edges_removed > 0) {
        //Can make the program a little faster by updating mb graph dynamically!

        if(is_colourable()) {
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

/******************************************************************************/

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

        int is_col = is_colourable();

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

        //printgraph_minibaum(current_graph_mb, adj_mb, nv + 2);
        if(chromatic_number_same_colour_vertices(current_graph_mb, adj_mb, num_colours,
                                                 isolated_vertices) == 0) {
            //i.e. there is no valid colouring where avoided_vertex + 1 and nv have the same colour
            //So the similar element is not destroyed!
            //Also in larger extensions they can't have the same colour, so:
            //previous_extension_was_not_colourable = 1;
            return 0;
        } else
            return 1;
    } else
        return 1;
}

/******************************************************************************/

/**
 * Returns 1 if all previously destroyed vertices are still destroyed, else returns 0.
 */
//Important: the removed vertices for which the colour dom graphs are computed are
//marked with MARKS2
static int
previously_destroyed_similar_vertices_are_still_destroyed() {
    //compute_colour_domination_graphs_vertex_removed();
    RESETMARKS2;

    int i;
    for(i = 0; i < num_destroyed_similar_vertices; i++) {
        if(!ISMARKED2(destroyed_similar_vertices[i][0])) {
            MARK2(destroyed_similar_vertices[i][0]);
            compute_colour_domination_graphs_vertex_removed_given(destroyed_similar_vertices[i][0]);
        }
        if(NBH_IS_SUBSET_OF_COL_DOM_VERTEX_REMOVED(destroyed_similar_vertices[i][0],
                                                   destroyed_similar_vertices[i][1]))
            return 0;
    }

    return 1;
}

/******************************************************************************/

/**
 * Adds the similar vertices to the list of destroyed_similar_vertices.
 */
static void
add_similar_vertices_to_list(unsigned char small_similar_vertex,
                             unsigned char larger_similar_vertex) {
    //Note that the vertices cannot already be in the list, otherwise the graph would have been rejected earlier!
    destroyed_similar_vertices[num_destroyed_similar_vertices][0] = small_similar_vertex;
    destroyed_similar_vertices[num_destroyed_similar_vertices][1] = larger_similar_vertex;
    num_destroyed_similar_vertices++;
}

/******************************************************************************/

static void
make_all_possible_sets_forced_colours(unsigned char possible_vertices[], int possible_vertices_size, int current_index,
                                      unsigned char selected_vertices[], int selected_vertices_size,
                                      int desired_selected_vertices_size, unsigned char given_colours[]) {

    if(valid_colouring_found_forced_colours)
        return;

    if(selected_vertices_size == desired_selected_vertices_size) {
        //Give selected_vertices[i]+1 colour i+1
        //And then test if the graph can be coloured...
        if(chromatic_number_given_startcolouring(current_graph_mb, adj_mb, num_colours,
                                                 selected_vertices, selected_vertices_size, given_colours) != 0) {
            //i.e. there is a colouring where the vertices in selected_vertices[] all have a different colour
            valid_colouring_found_forced_colours = 1;
        }

    } else {
        int i;
        for(i = current_index; i < possible_vertices_size; i++) {
            selected_vertices[selected_vertices_size] = possible_vertices[i];
            make_all_possible_sets_forced_colours(possible_vertices, possible_vertices_size, i + 1,
                                                  selected_vertices, selected_vertices_size + 1, desired_selected_vertices_size, given_colours);
        }

    }
}

/******************************************************************************/

static void
colour_vertices_in_all_possible_ways(unsigned char vertices[], int vertices_size, int current_index,
                                     unsigned char colours[], int num_desired_colours, setword forbidden_colour_vertices[],
                                     setword colours_already_used) {

    if(valid_colouring_found_forced_colours)
        return;

    if(current_index == vertices_size) {
        if(POPCOUNT(colours_already_used) == num_desired_colours &&
           chromatic_number_given_startcolouring(current_graph_mb, adj_mb, num_colours,
                                                 vertices, vertices_size, colours) != 0) {
            //i.e. there is a colouring where the vertices in vertices[] have num_desired_colours colours
            valid_colouring_found_forced_colours = 1;
        }
    } else {
        int i;
        for(i = 0; i < num_desired_colours; i++) {
            if(!ISELEMENT1(&forbidden_colour_vertices[i + 1], vertices[current_index])) {
                colours[current_index] = i + 1;

                setword forbidden_colour_vertices_backup = forbidden_colour_vertices[i + 1];
                forbidden_colour_vertices[i + 1] |= nautyg[vertices[current_index]]; //neighbours of vertex are not allowed

                setword colours_already_used_backup = colours_already_used;
                ADDELEMENT1(&colours_already_used, i+1);

                colour_vertices_in_all_possible_ways(vertices, vertices_size, current_index + 1,
                                                     colours, num_desired_colours, forbidden_colour_vertices, colours_already_used);

                colours_already_used = colours_already_used_backup;

                forbidden_colour_vertices[i + 1] = forbidden_colour_vertices_backup;
            }
        }
    }
}

/******************************************************************************/

//Determines the max num of colours used in any valid colouring of N(v) in G-v
//At most given_max_colours are used
//Note that G-v could be uncolourable (in that case 1 is returned)
static int
max_colours_in_valid_neighbourhood_colouring(unsigned char vertex, int given_max_colours) {
    //It is assumed that there is no valid colouring of N(v) in G-v with num_colours
    int max_num_colours = MIN(given_max_colours, adj[vertex]);

    if(max_num_colours > 1) {
        setword removed_vertices = 0;
        ADDELEMENT1(&removed_vertices, vertex);

        make_graph_with_removed_vertices(removed_vertices);
    }

    //Vertices which are not allowed to have a given colour
    setword forbidden_colour_vertices[max_num_colours + 1];
    int i;
    for(i = 1; i <= max_num_colours; i++)
        forbidden_colour_vertices[i] = 0;

    valid_colouring_found_forced_colours = 0;
    setword colours_already_used = 0;
    ADDELEMENT1(&colours_already_used, 1);

    unsigned char colours[adj[vertex]];
    while(max_num_colours > 1) {
        //if valid colouring found -> stop

        //Can actually fix first vertex, since other combinations are isomorphic!
        //Can actually fix more in case of > 2 colours, but probably won't make any difference?
        colours[0] = 1;
        setword forbidden_colour_vertices_backup = forbidden_colour_vertices[1];
        forbidden_colour_vertices[1] |= nautyg[current_graph[vertex][0]]; //neighbours of vertex are not allowed to have colour 1
        colour_vertices_in_all_possible_ways(current_graph[vertex], adj[vertex], 1,
                                             colours, max_num_colours, forbidden_colour_vertices, colours_already_used);
        //colour_vertices_in_all_possible_ways(current_graph[vertex], adj[vertex], 0,
        //    colours, max_num_colours);

        forbidden_colour_vertices[1] = forbidden_colour_vertices_backup;

        if(valid_colouring_found_forced_colours)
            return max_num_colours;

        max_num_colours--;
    }

    return 1; //It is assumed that G-v is colourable, so then there will be a colouring with 1 colour

}

/******************************************************************************/

//Test if there is a valid colouring of G-v where vertices of N(v) are coloured with k colours
//for every v in V(G)
//Returns 0 if it is the case, else returns 1
//If 1 is returned, best_small_vertex points to the *best* small vertex
//If 1 is returned, this means that G cannot be critical and that best_small_vertex must be destroyed
static int
contains_small_colour_dominated_vertex(unsigned char *best_small_vertex) {
    int i;
    //To avoid testing small vertices!
    for(i = 0; i < nv; i++)
        if(adj[i] < num_colours) {
            fprintf(stderr, "Error: contains small vertex\n");
            exit(1);
        }

    num_small_vertices = 0;

    //Min deg is clearly best!
    unsigned char degree_current_best_small = MAXN;
    unsigned char current_best_small;

    unsigned char given_colours[num_colours];
    for(i = 0; i < num_colours; i++)
        given_colours[i] = i+1;

    //Test if there is a colouring of G-v where vertices of N(v) are coloured with k colours
    for(i = 0; i < nv; i++) {
        setword removed_vertices = 0;
        ADDELEMENT1(&removed_vertices, i);

        make_graph_with_removed_vertices(removed_vertices);

        valid_colouring_found_forced_colours = 0;

        unsigned char selected_vertices[num_colours];
        make_all_possible_sets_forced_colours(current_graph[i], adj[i], 0, selected_vertices,
                                              0, num_colours, given_colours);

        if(!valid_colouring_found_forced_colours) {
            small_vertices[num_small_vertices++] = i;
            if(adj[i] < degree_current_best_small) {
                current_best_small = i;
                degree_current_best_small = adj[i];
            }
        }

    }

    if(degree_current_best_small < MAXN) {
        *best_small_vertex = current_best_small;
        return 1;
    } else
        return 0;
}

/******************************************************************************/

/**
 * Returns the number of edges of the current graph.
 */
static int
determine_number_of_edges() {
    int ne = 0;
    int i;
    for(i = 0; i < nv; i++)
        ne += adj[i];

    return ne / 2;
}

/******************************************************************************/

/**
 * Returns 1 if the current graph is planar, else returns 0.
 *
 * Uses planarity.c - code for planarity testing of undirected graphs by
 * Boyer and Myrvold.
 */
static int
is_planar() {
    int ne = determine_number_of_edges();
    //Never happens (because of previous_extension_was_not_colourable = 1)
    //if(ne > (3*nv - 6)) {
    //    fprintf(stderr, "cant be planar\n");
    //    return 0; //Can't be planar in this case
    //}

    sparsegraph sg;
    SG_INIT(sg);
    SG_ALLOC(sg, nv, 2*ne, "malloc");
    nauty_to_sg(nautyg, &sg, 1, nv);

/*
    printgraph(current_graph, adj, nv);
    print_sparse_graph_nauty(sg);
    fprintf(stderr, "\n");
*/

/*
    DYNALLSTAT(t_ver_sparse_rep,V,V_sz);
    DYNALLSTAT(t_adjl_sparse_rep,A,A_sz);
    DYNALLOC1(t_ver_sparse_rep, V, V_sz, nv, "planarg");
    DYNALLOC1(t_adjl_sparse_rep, A, A_sz, 2 * ne + 1, "planarg");
*/
    t_ver_sparse_rep V[nv];
    t_adjl_sparse_rep A[2*ne + 1];

    int k = 0;
    int i, j;
    for(i = 0; i < nv; ++i)
        if(sg.d[i] == 0)
            V[i].first_edge = NIL;
        else {
            V[i].first_edge = k;
            for(j = sg.v[i]; j < sg.v[i] + sg.d[i]; ++j) {
                A[k].end_vertex = sg.e[j];
                A[k].next = k + 1;
                ++k;
            }
            A[k - 1].next = NIL;
        }

    if(k != 2 * ne) {
        fprintf(stderr, "Error: decoding error (planarg)\n");
        exit(1);
    }

    SG_FREE(sg);

    int nbr_c;
    int edge_pos, v, w;
    t_dlcl **dfs_tree, **back_edges, **mult_edges;
    t_ver_edge *embed_graph;

    //TODO: possible optimisation: only start from last vertex?
    int ans = sparseg_adjl_is_planar(V, nv, A, &nbr_c,
                                     &dfs_tree, &back_edges, &mult_edges,
                                     &embed_graph, &edge_pos, &v, &w);


    //Don't forget this otherwise out of mem!
    sparseg_dlcl_delete(dfs_tree, nv);
    sparseg_dlcl_delete(back_edges, nv);
    sparseg_dlcl_delete(mult_edges, nv);
    embedg_VES_delete(embed_graph, nv);

    return ans;


}

/******************************************************************************/

/**
 * Returns 1 if there is an expansion without any children, else returns 0.
 *
 * Important: in case 1 is returned and in case the expansion has uncolourable children,
 * it is assumed that these uncolourable children were already output!
 */
static int contains_expansion_without_any_children() {

    unsigned char current_set[MAXN];

    //Test if contains similar vertices
    //Choose a given sim vertex and only perform expansions which destroy that one
    unsigned char best_small_similar_vertex;
    unsigned char other_larger_similar_vertex;

    int least_num_generated_from_parent = INT_MAX;

    if(!test_girth && contains_similar_vertices(&best_small_similar_vertex, &other_larger_similar_vertex)) {
        //Only search for expansions which destroy best_similar_vertex
        int i;
        for(i = 0; i < num_similar_vertices; i++) {
            number_of_graphs_generated_from_parent = 0;
            number_of_uncol_graphs_generated_from_parent = 0;

            current_set[0] = similar_vertices[i][0];
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, similar_vertices[i][0]);
            ADDELEMENT1(&forbidden_vertices, similar_vertices[i][1]);

            setword isolated_vertices = 0;
            ADDELEMENT1(&isolated_vertices, similar_vertices[i][0]);
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                isolated_vertices, similar_vertices[i][1], 0, 0, 0, 0);

            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                //Opmerking: ipv opslaan event al doen in contains_similar_vertices_col_dom()?
                if(number_of_graphs_generated_from_parent == 0) {
                    times_no_children_sim_vert[nv]++;
                    if(number_of_uncol_graphs_generated_from_parent > 0)
                        //Now do perform expansion so the valid uncol graphs are output!
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                            isolated_vertices, similar_vertices[i][1], 0, 0, 0, 1);

                    return 1; //Can't become better than this!
                }

                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
            }
        }
        times_children_sim_vert[nv]++;
    }

    //Only compute this when we really need to!
    //Was already partially computed in previously_destroyed_similar_vertices_are_still_destroyed()
    //Only compute the ones which were not yet computed in previously_destroyed_similar_vertices_are_still_destroyed()
    if(!test_girth && !test_if_contains_induced_fork) {
        RESETMARKS2;
        compute_colour_domination_graphs_vertex_removed();
    }
    if(!test_girth && !test_if_contains_induced_fork && contains_similar_vertices_col_dom(&best_small_similar_vertex, &other_larger_similar_vertex)) {
        int i;
        for(i = 0; i < num_similar_vertices; i++) {
            number_of_graphs_generated_from_parent = 0;
            number_of_uncol_graphs_generated_from_parent = 0;

            current_set[0] = similar_vertices[i][0];
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, similar_vertices[i][0]);
            ADDELEMENT1(&forbidden_vertices, similar_vertices[i][1]);

            setword isolated_vertices = 0;
            ADDELEMENT1(&isolated_vertices, similar_vertices[i][0]);
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                isolated_vertices, similar_vertices[i][1], 0, 0, 0, 0);

            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                if(number_of_graphs_generated_from_parent == 0) {
                    times_no_children_sim_vert_col[nv]++;
                    if(number_of_uncol_graphs_generated_from_parent > 0)
                        //Now do perform expansion so the valid uncol graphs are output!
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                            isolated_vertices, similar_vertices[i][1], 0, 0, 0, 1);

                    return 1; //Can't become better than this!
                }

                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
            }
        }
        times_children_sim_vert_col[nv]++;
    }

    //Testing small vertex first doesn't seem to help/matter much!

    //Warning: for diamondfree case small vertex needs higher priority
    //than similar edges in order to allow termination!
    unsigned char best_small_vertex = MAXN;
    //if only >= 4 then N3P6C3 never terminates!
    if(contains_too_small_vertex(&best_small_vertex)) {

        int i;
        for(i = 0; i < num_small_vertices; i++) {
            number_of_graphs_generated_from_parent = 0;
            number_of_uncol_graphs_generated_from_parent = 0;

            //Small vertices are nearly always correctly destroyed, so this test is not really useful!
            int max_colours_nbh_colouring_new = 0;
            if(nv > 1) //Else stuck when just 1 vertex
                max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(small_vertices[i], adj[small_vertices[i]]);

            current_set[0] = small_vertices[i];
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, small_vertices[i]);

            setword isolated_vertices = 0;
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                forbidden_vertices, isolated_vertices, 0,
                                                                max_colours_nbh_colouring_new, small_vertices[i], 0, 0);

            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                if(number_of_graphs_generated_from_parent == 0) {
                    times_no_children_small_vert[nv]++;

                    if(number_of_uncol_graphs_generated_from_parent > 0)
                        //Now do perform expansion so the valid uncol graphs are output!
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                            forbidden_vertices, isolated_vertices, 0,
                                                                            max_colours_nbh_colouring_new, small_vertices[i], 0, 1);

                    return 1; //Can't become better than this!
                }

                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
            }
        }
        times_children_small_vert[nv]++;

    } else if(contains_small_colour_dominated_vertex(&best_small_vertex)) {
        //Should only be called if no small vertices
        int i;
        for(i = 0; i < num_small_vertices && least_num_generated_from_parent > 0; i++) {
            number_of_graphs_generated_from_parent = 0;
            number_of_uncol_graphs_generated_from_parent = 0;

            //It is known that there is no valid colouring with num_colours!
            int max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(small_vertices[i], num_colours - 1);

            current_set[0] = small_vertices[i];
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, small_vertices[i]);

            setword isolated_vertices = 0;
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                forbidden_vertices, isolated_vertices, 0,
                                                                max_colours_nbh_colouring_new, small_vertices[i], 0, 0);

            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                if(number_of_graphs_generated_from_parent == 0) {
                    times_no_children_small_vert_col[nv]++;

                    if(number_of_uncol_graphs_generated_from_parent > 0)
                        //Now do perform expansion so the valid uncol graphs are output!
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                            forbidden_vertices, isolated_vertices, 0,
                                                                            max_colours_nbh_colouring_new, small_vertices[i], 0, 1);

                    return 1; //Can't become better than this!

                }
                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
            }
        }
        //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
        times_children_small_vert_col[nv]++;
    }


    //Crown lemmas only work in case of 3 colours!
    if(num_colours == 3) {
        setword crown_vertices;
        if(contains_odd_crown(&crown_vertices)) {
            int i;
            int least_num_generated_from_parent = INT_MAX;
            for(i = 0; i < num_cycles_stored; i++) {
                number_of_graphs_generated_from_parent = 0;
                number_of_uncol_graphs_generated_from_parent = 0;

                setword isolated_vertices = 0;
                setword forbidden_vertices = 0;
                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                    0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 0);

                if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                    if(number_of_graphs_generated_from_parent == 0) {
                        times_no_children_odd_crown[nv]++;
                        if(number_of_uncol_graphs_generated_from_parent > 0) {
                            //Now do perform expansion so the valid uncol graphs are output!
                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 1);
                        }

                        return 1; //Can't become better than this!
                    }

                    least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                }
            }
            //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
            times_children_odd_crown[nv]++;

        }

        if(contains_even_crown(&crown_vertices)) {
            int i;
            int least_num_generated_from_parent = INT_MAX;
            for(i = 0; i < num_cycles_stored; i++) {
                number_of_graphs_generated_from_parent = 0;
                number_of_uncol_graphs_generated_from_parent = 0;

                setword isolated_vertices = 0;
                setword forbidden_vertices = 0;
                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                    0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 0);

                if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                    if(number_of_graphs_generated_from_parent == 0) {
                        times_no_children_even_crown[nv]++;
                        if(number_of_uncol_graphs_generated_from_parent > 0) {
                            //Now do perform expansion so the valid uncol graphs are output!
                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 1);
                        }

                        return 1; //Can't become better than this!
                    }

                    least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                }
            }
            //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
            times_children_even_crown[nv]++;
        }
    }

    //It is much faster not to test if sim ep's have 0 children, since this hardly never happens!
    //Or even never?

    return 0;
}

/******************************************************************************/

/**
 * DFS algorithm which recursively marks all vertices which can be reached from
 * current_vertex.
 * Complexity: O(|V|+|E|)
 */
static void mark_dfs(unsigned char current_vertex) {
    MARK(current_vertex);
    ADDELEMENT1(&current_component, current_vertex);
    number_of_vertices_marked++;

    int i;
    for(i = 0; i < adj[current_vertex]; i++)
        if(!ISMARKED(current_graph[current_vertex][i]))
            mark_dfs(current_graph[current_vertex][i]);

}

/******************************************************************************/

/**
 * Returns the number of connectivity components in the graph when the vertices
 * in the bitvector forbidden_vertices are removed.
 */
static int
number_of_components_when_vertices_removed(setword forbidden_vertices) {
    RESETMARKS;
    number_of_vertices_marked = 0;

    //First mark forbidden vertices so they won't be visited
    int i = -1;
    while((i = nextelement(&forbidden_vertices, 1, i)) >= 0) {
        MARK(i);
        number_of_vertices_marked++;
    }

    int number_of_components = 0;
    for(i = 0; i < nv; i++)
        if(!ISMARKED(i)) {
            int number_of_vertices_marked_before = number_of_vertices_marked;
            current_component = 0;
            mark_dfs(i);
            number_of_components++;
            int num_elements_in_component = number_of_vertices_marked - number_of_vertices_marked_before;
            if(num_elements_in_component < best_number_of_elements_in_component) {
                best_number_of_elements_in_component = num_elements_in_component;
                best_component = current_component;
            }
        }

    if(number_of_vertices_marked != nv) {
        fprintf(stderr, "Error: not all vertices were marked!\n");
        exit(1);
    }

    return number_of_components;
}

/******************************************************************************/

/**
 * Returns 1 if the graph contains a cutvertex, else returns 0.
 * If 1 is returned, best_cutvertex points to the *best* cutvertex and
 * number_of_components contains the number of connectivity components when
 * best_cutvertex is removed.
 */
static int
contains_cutvertex(unsigned char *best_cutvertex, int *number_of_components,
                   setword *best_comp) {
    //int number_of_cutvertices = 0;

    int best_number_of_elements_in_component_local = MAXN;

    int i;
    for(i = 0; i < nv; i++) {
        setword forbidden_vertices = 0;
        ADDELEMENT1(&forbidden_vertices, i);
        best_number_of_elements_in_component = MAXN;
        int num_comp = number_of_components_when_vertices_removed(forbidden_vertices);
        //if(num_comp > 1) {
        if(num_comp > 1 && best_number_of_elements_in_component < best_number_of_elements_in_component_local) {
            best_number_of_elements_in_component_local = best_number_of_elements_in_component;
            *best_cutvertex = i;
            *number_of_components = num_comp;
            *best_comp = best_component;
            //return 1;
        }
    }

    if(best_number_of_elements_in_component_local < MAXN)
        return 1;

    return 0;
}

/******************************************************************************/

/**
 * Searches for all possible sets of 4 vertices from candidate_vertices[] which
 * are no neighbours. Then tests for each quadruple if it can be extended to an induced fork.
 *
 * If induced_fork_found = 1 upon return, it means that the graph contains an
 * induced fork, else it doesn't.
 *
 * Note 1: Max number of possible quadruples = num_candidate_vertices choose 4
 * Note 2: In the triangle-free case all sets are of course valid (but this test is no bottleneck)
 */
static void find_all_isolated_quadruples(unsigned char candidate_vertices[], int num_candidate_vertices,
                                         int next_index, unsigned char current_set[], int current_set_size, setword forbidden_vertices, unsigned char central_vertex) {

    if(current_set_size == 4) {
/*
        int i;
        for(i = 0; i < current_set_size; i++)
            fprintf(stderr, "%d ", current_set[i]);
        fprintf(stderr, "\n");
*/
        total_quadruples_found++;

        setword valid_neighbours[current_set_size][MAXN];
        int num_valid_neigbhours[current_set_size];
        int i, j;
        for(i = 0; i < current_set_size; i++) {
            //First determine forbidden neighbours of current_set[i]
            setword forbidden_neighbours = nautyg[central_vertex];
            for(j = 0; j < current_set_size; j++)
                if(j != i)
                    forbidden_neighbours |= nautyg[current_set[j]];

            num_valid_neigbhours[i] = 0;
            for(j = 0; j < adj[current_set[i]]; j++)
                if(!ISELEMENT1(&forbidden_neighbours, current_graph[current_set[i]][j])) {
                    valid_neighbours[i][num_valid_neigbhours[i]] = current_graph[current_set[i]][j];
                    num_valid_neigbhours[i]++;
                }
        }

        //Ok, now check if we can choose a pair of valid neighbours which are no neighbours
        //of each other
        for(i = 0; i < current_set_size; i++)
            for(j = i + 1; j < current_set_size; j++) {
                //Test if current_set[i] and current_set[j] have any valid neighbours
                //which are no neighbours of each other
                int k, l;
                for(k = 0; k < num_valid_neigbhours[i]; k++)
                    for(l = 0; l < num_valid_neigbhours[j]; l++)
                        if(!ISELEMENT1(&nautyg[valid_neighbours[i][k]], valid_neighbours[j][l])) {
                            induced_fork_found = 1;
                            return;
                        }
            }


    } else {
        int i;
        for(i = next_index; i < num_candidate_vertices; i++) {
            //Could also stop if 4 elements can no longer be reached, but doesn't make any difference

            //Remark: forbidden_vertices not necessary in triangle-free case, but is no bottleneck!

            if(!ISELEMENT1(&forbidden_vertices, candidate_vertices[i])) {
                current_set[current_set_size] = candidate_vertices[i];
                //neighbourhood of current vertex is forbidden!
                find_all_isolated_quadruples(candidate_vertices, num_candidate_vertices, i + 1,
                                             current_set, current_set_size + 1, forbidden_vertices | nautyg[candidate_vertices[i]], central_vertex);

                if(induced_fork_found)
                    return;

            }

        }
    }

}

/******************************************************************************/

static int triangle_combination_leads_to_bull(int a0, int a1, int a2) {


    int i;
    for(i = 0; i < adj[a0]; i++) {
        //First horn from a0
        int b1 = current_graph[a0][i];
        setword forbidden_nbrs_first_horn = 0;
        ADDELEMENT1(&forbidden_nbrs_first_horn, a1);
        ADDELEMENT1(&forbidden_nbrs_first_horn, a2);
        if(!ISELEMENT1(&forbidden_nbrs_first_horn, b1) && (forbidden_nbrs_first_horn & nautyg[b1]) == 0) {
            //Second horn from a1
            int j;
            for(j = 0; j < adj[a1]; j++) {
                int b2 = current_graph[a1][j];
                setword forbidden_nbrs_second_horn = 0;
                ADDELEMENT1(&forbidden_nbrs_second_horn, a0);
                ADDELEMENT1(&forbidden_nbrs_second_horn, a2);
                ADDELEMENT1(&forbidden_nbrs_second_horn, b1);
                if(!ISELEMENT1(&forbidden_nbrs_second_horn, b2) && (forbidden_nbrs_second_horn & nautyg[b2]) == 0)
                    return 1; //Induced bull found
            }
        }
    }

    return 0;

}

/**
 * Returns 1 if the graph contains an induced bull, else returns 0.
 */
//TODO: can remember previous induced bulls, but this method is no bottleneck?
static int contains_induced_bull() {

    //printgraph(current_graph, adj, nv);

    int i;
    for(i = 0; i < nv; i++) {
        set *gv = GRAPHROW1(nautyg, i, MAXM);

        //int neighbour_b1 = -1;
        int neighbour_b1 = i;
        while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
            setword intersect_neigh_n_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
            //int neighbour_b2 = -1;
            int neighbour_b2 = neighbour_b1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_n_b1, 1, neighbour_b2)) >= 0) {
                //Triangle i neighbour_b1 neighbour_b2 found with i < neighbour_b1 < neighbour_b2
                //fprintf(stderr, "%d %d %d\n", i, neighbour_b1, neighbour_b2);
                //3 possibilities: 0 1, 0 2, 1 2
                if(triangle_combination_leads_to_bull(i, neighbour_b1, neighbour_b2)
                   || triangle_combination_leads_to_bull(i, neighbour_b2, neighbour_b1)
                   || triangle_combination_leads_to_bull(neighbour_b1, neighbour_b2, i))
                    return 1;

            }
        }
    }

    return 0;
}

/******************************************************************************/

/**
 * Returns 1 if the graph contains an induced fork, else returns 0.
 * A fork is obtained from K_{1,4} by subdividing two edges (so it has 7 vertices).
 */
//TODO: can remember previous induced forks, but this method is no bottleneck?
static int contains_induced_fork() {
    unsigned char current_set[4];

    induced_fork_found = 0;

    int centervertex;
    for(centervertex = 0; centervertex < nv; centervertex++)
        if(adj[centervertex] > 3) { //Centervertex has degree >= 4
            //For all combinations of 4 neighbours of centervertex which are no neighbours

            //fprintf(stderr, "Vertex with degree %d\n", adj[centervertex]);

            total_quadruples_found = 0;
            find_all_isolated_quadruples(current_graph[centervertex], adj[centervertex],
                                         0, current_set, 0, 0, centervertex);

            //fprintf(stderr, "total_sets_found: %d\n", total_sets_found);
            //fprintf(stderr, "\n");

            if(induced_fork_found)
                return 1;
        }

    return 0;
}


/******************************************************************************/

static int
test_if_combination_yields_gem(TRIANGLE triangle, int central_vertex, int central_vertex_index) {

    EDGE edge0;
    edge0[0] = central_vertex;
    edge0[1] = triangle[(central_vertex_index + 1) % 3];
    int edge0_third_vertex = triangle[(central_vertex_index + 2) % 3];

    EDGE edge1;
    edge1[0] = central_vertex;
    edge1[1] = edge0_third_vertex;
    int edge1_third_vertex = edge0[1];

    setword intersect_neigh_edge0 = *GRAPHROW1(nautyg, edge0[0], MAXM) & *GRAPHROW1(nautyg, edge0[1], MAXM);
    //int neighbour_b2 = -1;
    int extra_triangle_vertex_edge0 = -1;
    while((extra_triangle_vertex_edge0 = nextelement1(&intersect_neigh_edge0, 1, extra_triangle_vertex_edge0)) >= 0)
        if(extra_triangle_vertex_edge0 != edge0_third_vertex
           && !ISELEMENT1(GRAPHROW1(nautyg, edge0_third_vertex, MAXM), extra_triangle_vertex_edge0)) {
            //!ISELEMENT1(GRAPHROW1(nautyg, i, MAXM), j)

            //Now second triangle
            setword intersect_neigh_edge1 = *GRAPHROW1(nautyg, edge1[0], MAXM) & *GRAPHROW1(nautyg, edge1[1], MAXM);
            int extra_triangle_vertex_edge1 = -1;
            while((extra_triangle_vertex_edge1 = nextelement1(&intersect_neigh_edge1, 1, extra_triangle_vertex_edge1)) >= 0)
                if(extra_triangle_vertex_edge1 != edge1_third_vertex
                   && !ISELEMENT1(GRAPHROW1(nautyg, edge1_third_vertex, MAXM), extra_triangle_vertex_edge1)
                   && !ISELEMENT1(GRAPHROW1(nautyg, extra_triangle_vertex_edge0, MAXM), extra_triangle_vertex_edge1)) {
                    return 1; //Induced gem found
                }
        }

    return 0;
}


/******************************************************************************/

/**
 * Returns 1 if the graph contains an induced gem, else returns 0.
 */
//Graph6 code gem: Dh{
//Since the parent graph was gem-free, the last vertex must be in the gem
//But not doing this since this is a bit tricky as a gem has 3 vertex-orbits
static int
contains_induced_gem() {
    //Don't assume that the last vertex must be in it, since it can be tricky
    //Just find every triangle and test if they can be the central triangle of a gem

    int i;
    for(i = 0; i < nv; i++) {
        set *gv = GRAPHROW1(nautyg, i, MAXM);
        //int neighbour_b1 = -1;
        int neighbour_b1 = i;
        while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
            setword intersect_neigh_i_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
            //int neighbour_b2 = -1;
            int neighbour_b2 = neighbour_b1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_i_b1, 1, neighbour_b2)) >= 0) {
                //Triangle i neighbour_b1 neighbour_b2 found with i < neighbour_b1 < neighbour_b2
                //fprintf(stderr, "%d %d %d\n", i, neighbour_b1, neighbour_b2);

                TRIANGLE triangle;
                triangle[0] = i;
                triangle[1] = neighbour_b1;
                triangle[2] = neighbour_b2;

                //3 possibilities for central vertex of the gem
                if(test_if_combination_yields_gem(triangle, triangle[0], 0)
                   || test_if_combination_yields_gem(triangle, triangle[1], 1)
                   || test_if_combination_yields_gem(triangle, triangle[2], 2))
                    return 1; //Induced gem found

            }
        }
    }

    return 0;

}


// MA. Checks if graph contains_induced_copy
/******************************************************************************/
/*
 * Returns 1 if the graph contains an induced copy, else returns 0.
 */
 
static int
contains_induced_copy() {

    int i;
    for(i = 0; i < nv; i++) {
        set *gv = GRAPHROW1(nautyg, i, MAXM);
        //int neighbour_b1 = -1;
        int neighbour_b1 = i;
        while((neighbour_b1 = nextelement1(gv, 1, neighbour_b1)) >= 0) {
            setword intersect_neigh_i_b1 = *GRAPHROW1(nautyg, neighbour_b1, MAXM) & *gv;
            //int neighbour_b2 = -1;
            int neighbour_b2 = neighbour_b1;
            while((neighbour_b2 = nextelement1(&intersect_neigh_i_b1, 1, neighbour_b2)) >= 0) {
                //Triangle i neighbour_b1 neighbour_b2 found with i < neighbour_b1 < neighbour_b2
                
                fprintf(stderr, "%d %d %d\n", i, neighbour_b1, neighbour_b2);

                TRIANGLE triangle;
                triangle[0] = i;
                triangle[1] = neighbour_b1;
                triangle[2] = neighbour_b2;

                //3 possibilities for central vertex of the gem
                if(test_if_combination_yields_gem(triangle, triangle[0], 0)
                   || test_if_combination_yields_gem(triangle, triangle[1], 1)
                   || test_if_combination_yields_gem(triangle, triangle[2], 2))
                    return 1; //Induced gem found

            }
        }
    }

    return 0;

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
static void
extend(setword isolated_vertices, unsigned char avoided_vertex,
       int max_colours_nbh_colouring, unsigned char destroyed_small_vertex, int perform_extensions) {
    //Can now happen since starting from multiple startgraphs
    if(nv > maxnv) {
        fprintf(stderr, "Error: nv is too big: %d vs max: %d\n", nv, maxnv);
        exit(1);
    }

    num_graphs_generated[nv]++;

    if(test_if_trianglefree) {
        if(contains_triangle()) {
            //Actually not really necessary to test this anymore, since it is already tested in construct_and_expand_vertex_sets_forbidden_vertices
            //But is no bottleneck

            //If contains triangle, next extensions will also contain triangle!
            previous_extension_was_not_colourable = 1;
            counts_contains_triangle[nv]++;
            return;
        } else
            counts_doesnt_contain_triangle[nv]++;
    } else if(test_if_diamondfree) {
        //if(contains_diamond()) {
        //if(contains_diamond_induced()) {
        //contains_diamond_induced() is much slower than contains_diamond(), so use contains_diamond() when generating K4-free graphs
        if((test_if_K4free && contains_diamond()) || (!test_if_K4free && contains_diamond_induced())) {
            //K4 doesn't contain diamond as induced subgraph!
            //Other graphs may contain no induced diamond but a K4, but these
            //graphs cannot be 4-critical since they contain a K4, so it's ok
            //that these graphs are rejected as well!
            counts_contains_diamond[nv]++;
            //Niet previous_extension_was_not_colourable = 1; want kan zijn dan plots niet meer induced als extra edge!

            if(test_if_K4free) {
                if(!(nv == 4 && contains_K4())) {
                    //counts_contains_diamond[nv]++;
                    //if(nv != 4) //Else K4 is not found
                    previous_extension_was_not_colourable = 1; //Set this to 1, much faster!
                    //return;
                }
            }

            return;
        } else
            counts_doesnt_contain_diamond[nv]++;
    }

    if(!test_if_trianglefree && test_if_K4free) {
        if(nv >= 4 && contains_K4()) {
            counts_contains_K4[nv]++;
            //If contains triangle, next extensions will also contain triangle!
            previous_extension_was_not_colourable = 1;
            return;
        } else
            counts_doesnt_contain_K4[nv]++;
    }

    if(!test_if_trianglefree && !test_if_K4free && test_if_K5free) {
        if(nv >= 5 && contains_K5()) {
            counts_contains_K5[nv]++;
            //If contains triangle, next extensions will also contain triangle!
            previous_extension_was_not_colourable = 1;
            return;
        } else
            counts_doesnt_contain_K5[nv]++;
    }

    if(test_girth && girth > 4) {
        //if(0 && contains_forbidden_induced_cycle(4)) {
        if(countains_fourcycle()) {
            //Actually not really necessary to test this anymore, since it is already tested in construct_and_expand_vertex_sets_forbidden_vertices
            //But is no bottleneck

            previous_extension_was_not_colourable = 1;
            return; //prune
        }
        if(girth > 5 && contains_forbidden_induced_cycle(5)) {
            previous_extension_was_not_colourable = 1;
            return; //prune
        }
        if(girth > 6 && contains_forbidden_induced_cycle(6)) {
            previous_extension_was_not_colourable = 1;
            return; //prune
        }
        if(girth > 7) {
            fprintf(stderr, "Error: max supported girth is 7 for now\n");
            exit(1);
        }
    }

    //It is a lot faster to test if the graph contains a K4 before calling nauty!

    //The fastest option seems to be: K4, C5, W5, nauty, P-free
    //But K4, W5, C5, nauty, P-free is hardly any slower
    //Remark: may depend on specific parameters, so test for larger cases!

    if(num_colours == 3 && !test_if_diamondfree && !test_if_trianglefree) {
        //Test if the graph contains a K4
        //If it contains a K4 it won't be a minimal not 3-col graph, so we can already
        //prune without testing if it is P-free
        //If the graph is diamond-free it will certainly also be K4-free
        if(nv > 4 && contains_K4()) { //The last vertex must be in the K4, since the parents were K4-free
            counts_contains_K4[nv]++;
            previous_extension_was_not_colourable = 1;
            return; //prune
        } else
            counts_doesnt_contain_K4[nv]++;
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

    //Maybe more efficient to test this sooner/later?
    if(test_if_planar) {
        if(!is_planar()) {
            times_graph_isnt_planar[nv]++;
            //If this extension is not planar, next extensions also won't be planar!
            previous_extension_was_not_colourable = 1;
            return; //prune
        } else {
            times_graph_is_planar[nv]++;
        }
    }

    if(num_colours == 4 && !test_if_diamondfree && !test_if_trianglefree) {
        //This test doesn't really seem to help much since most graphs don't contain a K5 (but ratio is increasing)
        if(nv > 5 && contains_K5()) { //The last vertex must be in the K4, since the parents were K4-free
            counts_contains_K5[nv]++;
            previous_extension_was_not_colourable = 1;
            return; //prune
        } else
            counts_doesnt_contain_K5[nv]++;
    }

    //Actually performing induced C5 test in contains_W5(), so if the current graph
    //doesn't contain an induced C5, it also won't contain a W5, so no need to test that (certainly helps)!
    if(num_colours == 3 && !test_if_diamondfree && !test_if_trianglefree &&
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

    //P-free test is much more expensive than col test, but nearly all graphs are colourable for most cases
    //so better to do P-free test first

    //It is faster to do P-free test before isomorphism test
    //(certainly for large graphs, otherwise too much graphs in mem and most are not isomorphic)
    if(!test_if_contains_induced_fork && contains_forbidden_induced_path()) {
        times_graph_not_Pfree[nv]++;
        return; //prune since if current_graph contains a forbidden induced path, so will it's children
    } else
        times_graph_Pfree[nv]++;


    if(test_if_contains_induced_fork) {
        if(contains_induced_fork()) {
            times_graph_not_forkfree[nv]++;
            return; //not fork-free
        } else
            times_graph_forkfree[nv]++;

    }


    if(test_if_contains_induced_bull) {
        if(contains_induced_bull()) {
            times_graph_not_bullfree[nv]++;
            return; //not bull-free
        } else
            times_graph_bullfree[nv]++;

    }

    if(test_if_contains_induced_gem) {
        if(contains_induced_gem()) {
            times_graph_not_gemfree[nv]++;
            return; //not gem-free
        } else
            times_graph_gemfree[nv]++;

    }


    // MA. test if graph contains an induced copy of any graph
    
    if(test_if_contains_induced_copy) {

        if(contains_induced_copy()) {
            times_graph_not_induced[nv]++;
            return; 
        } else
            times_graph_induced[nv]++;

    }

    







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

    if(max_colours_nbh_colouring > 0) {
        //TODO: first test if G-v is colourable
        //If not -> reject current graph (is also not colourable and cannot be critical)

        //If num_colours > 3: then it might be more efficient only to test if graph has colouring with
        //max_colours_nbh_colouring+1 colours

        //An expansion can increase max_colours_in_valid_neighbourhood_colouring() by at most 1
        if(max_colours_in_valid_neighbourhood_colouring(destroyed_small_vertex, max_colours_nbh_colouring + 1)
           <= max_colours_nbh_colouring) {
            //i.e. small vertex was not destroyed
            previous_extension_was_not_colourable = 1;
            //larger expansions also certainly won't destroy it
            times_small_vertex_not_destroyed[nv]++;
            return; //prune            
        } else
            times_small_vertex_destroyed[nv]++;

    }

    //Test this before calling nauty & splay
    //TODO: doesn't help much since most graphs at this point are colourable?
    //Is usually faster to do this before P-free test
    int is_col = is_colourable();
    if(!is_col) {
        times_graph_not_colourable[nv]++;

        //Very important: when using this opt, the col test must be performed
        //AFTER the P-free test!!! Since if this graph is not colourable 
        //but contains an induced Px, one of the other extensions might still yield a P-free critical graph!
        if(!output_vertex_critical_graphs)
            previous_extension_was_not_colourable = 1;
        //Now test if the graph is vertex-critical
        //We don't need to remove the last vertex since we know its parent is k-colourable

        //Prune if not vertex critical
        if(!is_vertex_critical()) {
            times_graph_not_vertex_critical[nv]++;

            return; //Prune, so we also don't need to call nauty
        } else
            times_graph_vertex_critical[nv]++;

    } else {
        times_graph_colourable[nv]++;
    }

    //Test if old similar vertices are still no similar vertices
    //Important: do this before isomorphism test otherwise not all graphs will be generated!
    //Note: can only be similar vertices again through hidden edges!
    if(0 && !previously_destroyed_similar_vertices_are_still_destroyed()) {
        times_similar_vertices_no_longer_destroyed[nv]++;
        //If similar vertices because of hidden edges, next expansion will also
        //contain at least the same hidden edges!
        previous_extension_was_not_colourable = 1;
        return;
    } else
        times_similar_vertices_are_still_destroyed[nv]++;

/*
    //Important: do this before splay, otherwise not all graphs will be found!
    if(!perform_extensions) {
        //Must also count uncol ones if returning when number_of_graphs_generated_from_parent == 0
        if(is_col)
            number_of_graphs_generated_from_parent++; //Number of non-iso P-free col graphs generated
        else
            number_of_uncol_graphs_generated_from_parent++;
        
        return;
    }        
*/


    //malloc is not faster, but it uses less memory than using MAXN!
    graph *can_form = (graph *) malloc(sizeof (graph) * nv);
    if(can_form == NULL) {
        fprintf(stderr, "Error: can't get more space\n");
        exit(1);
    }
    nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM, nv, can_form);
    //nauty(nautyg, lab, ptn, NULL, orbits, &options, &stats, workspace, WORKSIZE, MAXM, nv, nautyg_canon);    
    //memcpy(can_form, nautyg_canon, sizeof (graph) * nv);

    if(!perform_extensions) {
        int is_new_node; //Is not used here, but same args as splay_insert
        //TODO: drop is new node argument
        if(splay_lookup(&list_of_worklists[nv], can_form, &is_new_node) == NULL) { //i.e. is new node
            //Must also count uncol ones if returning when number_of_graphs_generated_from_parent == 0
            if(is_col)
                number_of_graphs_generated_from_parent++; //Number of non-iso P-free col graphs generated
            else
                number_of_uncol_graphs_generated_from_parent++;
        }
        free(can_form); //Don't forget to free!
        return;
    }

    int is_new_node;
    splay_insert(&list_of_worklists[nv], can_form, &is_new_node);
    if(!is_new_node) {
        num_graphs_generated_iso[nv]++;
        free(can_form); //Don't forget to free!
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

        return; //prune since if current_graph not k-col, its children also wont be k-colourable
    }

    //determine_connectivity_statistics();

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

#ifdef TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN
    //Before applying any of the other tests, first test if there is an expansion with 0 children...
    if(nv > CONTAINS_EXPANSION_WITHOUT_CHILDREN_LEVEL) {
        if(contains_expansion_without_any_children()) {
            times_expansion_with_no_children[nv]++;
            return;
        } else
            times_no_expansion_with_no_children[nv]++;
    }
#endif

    //TODO: possibly only do this from a certain nv or only if no regular sim vertices etc.
    //compute_colour_domination_graph();

    //Important: compute possible expansions on the fly! (Is faster + segfault if array becomes too big)
    unsigned char current_set[MAXN];

    //Test if contains similar vertices
    //Choose a given sim vertex and only perform expansions which destroy that one
    unsigned char best_small_similar_vertex;
    unsigned char other_larger_similar_vertex;
    //Remark: in N3P6 case it is better not to test contains_similar_vertices() separately
    //but immediately go to contains_similar_vertices_col_dom() instead!
    //The same goes for the N3P7C5 case!

    //Only do this in case no girth restrictions!
    if(!test_girth && contains_similar_vertices(&best_small_similar_vertex, &other_larger_similar_vertex)) {
        //Only search for expansions which destroy best_similar_vertex
        times_contains_similar_vertices[nv]++;

        if(nv >= LEVEL_LEAST_CHILDREN  && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
           && num_similar_vertices > 1) {
            int i;
            int least_num_generated_from_parent = INT_MAX;
            for(i = 0; i < num_similar_vertices && least_num_generated_from_parent > 0; i++) {
                number_of_graphs_generated_from_parent = 0;
                number_of_uncol_graphs_generated_from_parent = 0;

                current_set[0] = similar_vertices[i][0];
                setword forbidden_vertices = 0;
                ADDELEMENT1(&forbidden_vertices, similar_vertices[i][0]);
                ADDELEMENT1(&forbidden_vertices, similar_vertices[i][1]);

                setword isolated_vertices = 0;
                ADDELEMENT1(&isolated_vertices, similar_vertices[i][0]);
                construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                    isolated_vertices, similar_vertices[i][1], 0, 0, 0, 0);

                if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                    if(number_of_graphs_generated_from_parent == 0) {
                        times_no_children_sim_vert[nv]++;
                        if(number_of_uncol_graphs_generated_from_parent == 0)
                            return; //Can't become better than this!
                    }

                    least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                    best_small_similar_vertex = similar_vertices[i][0];
                    other_larger_similar_vertex = similar_vertices[i][1];
                }
                if(i == BREAK_VALUE_LEAST_CHILDREN)
                    break;
            }
            //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
            if(least_num_generated_from_parent > 0)
                times_children_sim_vert[nv]++;
        }

        add_similar_vertices_to_list(best_small_similar_vertex,
                                     other_larger_similar_vertex);

        current_set[0] = best_small_similar_vertex;
        setword forbidden_vertices = 0;
        ADDELEMENT1(&forbidden_vertices, best_small_similar_vertex);
        ADDELEMENT1(&forbidden_vertices, other_larger_similar_vertex);

        setword isolated_vertices = 0;
        ADDELEMENT1(&isolated_vertices, best_small_similar_vertex);
        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                            isolated_vertices, other_larger_similar_vertex, 0, 0, 0, 1);

        num_destroyed_similar_vertices--;

        //fprintf(stderr, "-nv: %d number_of_vertex_sets_global: %d\n", nv, number_of_vertex_sets_global);
    } else {
        times_doesnt_contain_similar_vertices[nv]++;

        //Only compute this when we really need to!
        //Was already partially computed in previously_destroyed_similar_vertices_are_still_destroyed()
        //Only compute the ones which were not yet computed in previously_destroyed_similar_vertices_are_still_destroyed()
#ifndef TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN
        if(!test_girth && !test_if_contains_induced_fork) {
            //Is already computed in contains_expansion_without_any_children() 
            RESETMARKS2;
            compute_colour_domination_graphs_vertex_removed();
        }
#else
        if(nv <= CONTAINS_EXPANSION_WITHOUT_CHILDREN_LEVEL) {
            if(!test_girth && !test_if_contains_induced_fork) {
                //Is already computed in contains_expansion_without_any_children() 
                RESETMARKS2;
                compute_colour_domination_graphs_vertex_removed();
            }
        }
#endif
        //Best nog to search for sim vertices with hidden edges if searching for fork-free graphs!
        if(!test_girth && !test_if_contains_induced_fork && contains_similar_vertices_col_dom(&best_small_similar_vertex, &other_larger_similar_vertex)) {
            times_contains_similar_vertices_col_dom[nv]++;

            if(nv >= LEVEL_LEAST_CHILDREN && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
               && num_similar_vertices > 1) {
                int i;
                int least_num_generated_from_parent = INT_MAX;
                for(i = 0; i < num_similar_vertices && least_num_generated_from_parent > 0; i++) {
                    number_of_graphs_generated_from_parent = 0;
                    number_of_uncol_graphs_generated_from_parent = 0;

                    current_set[0] = similar_vertices[i][0];
                    setword forbidden_vertices = 0;
                    ADDELEMENT1(&forbidden_vertices, similar_vertices[i][0]);
                    ADDELEMENT1(&forbidden_vertices, similar_vertices[i][1]);

                    setword isolated_vertices = 0;
                    ADDELEMENT1(&isolated_vertices, similar_vertices[i][0]);
                    construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                        isolated_vertices, similar_vertices[i][1], 0 , 0, 0, 0);

                    if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                        if(number_of_graphs_generated_from_parent == 0) {
                            times_no_children_sim_vert_col[nv]++;
                            if(number_of_uncol_graphs_generated_from_parent == 0)
                                return; //Can't become better than this!
                        }

                        least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                        best_small_similar_vertex = similar_vertices[i][0];
                        other_larger_similar_vertex = similar_vertices[i][1];
                    }
                    if(i == BREAK_VALUE_LEAST_CHILDREN)
                        break;
                }
                //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
                if(least_num_generated_from_parent > 0)
                    times_children_sim_vert_col[nv]++;
            }

            add_similar_vertices_to_list(best_small_similar_vertex,
                                         other_larger_similar_vertex);

            current_set[0] = best_small_similar_vertex;
            setword forbidden_vertices = 0;
            ADDELEMENT1(&forbidden_vertices, best_small_similar_vertex);
            ADDELEMENT1(&forbidden_vertices, other_larger_similar_vertex);

            setword isolated_vertices = 0;
            ADDELEMENT1(&isolated_vertices, best_small_similar_vertex);
            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, isolated_vertices, 0, forbidden_vertices,
                                                                isolated_vertices, other_larger_similar_vertex, 0, 0, 0, 1);

            num_destroyed_similar_vertices--;
        } else {
            times_doesnt_contain_similar_vertices_col_dom[nv]++;

            //Warning: for diamondfree case small vertex needs higher priority
            //than similar edges in order to allow termination!
            unsigned char best_small_vertex;
            //if only >= 4 then N3P6C3 never terminates!
            if(contains_too_small_vertex(&best_small_vertex)) {
                //if(num_colours >= 4 && contains_too_small_vertex(&best_small_vertex)) {
                times_contains_small_vertex[nv]++;

                if(nv >= LEVEL_LEAST_CHILDREN  && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
                   && num_small_vertices > 1) {
                    int i;
                    int least_num_generated_from_parent = INT_MAX;
                    for(i = 0; i < num_small_vertices && least_num_generated_from_parent > 0; i++) {
                        number_of_graphs_generated_from_parent = 0;
                        number_of_uncol_graphs_generated_from_parent = 0;

                        //Small vertices are nearly always correctly destroyed, so this test is not really useful!
                        int max_colours_nbh_colouring_new = 0;
                        if(nv > 1) //Else stuck when just 1 vertex
                            max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(small_vertices[i], adj[small_vertices[i]]);

                        current_set[0] = small_vertices[i];
                        setword forbidden_vertices = 0;
                        ADDELEMENT1(&forbidden_vertices, small_vertices[i]);

                        setword isolated_vertices = 0;
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                            forbidden_vertices, isolated_vertices, 0,
                                                                            max_colours_nbh_colouring_new, small_vertices[i], 0, 0);

                        if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                            if(number_of_graphs_generated_from_parent == 0) {
                                times_no_children_small_vert[nv]++;
                                if(number_of_uncol_graphs_generated_from_parent == 0) {
                                    return; //Can't become better than this!
                                }
                            }
                            least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                            best_small_vertex = small_vertices[i];
                        }
                        if(i == BREAK_VALUE_LEAST_CHILDREN)
                            break;
                    }
                    //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
                    if(least_num_generated_from_parent > 0)
                        times_children_small_vert[nv]++;
                }

                //Small vertices are nearly always correctly destroyed, so this test is not really useful!
                int max_colours_nbh_colouring_new = 0;
                if(nv > 1) //Else stuck when just 1 vertex
                    max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(best_small_vertex, adj[best_small_vertex]);

                current_set[0] = best_small_vertex;
                setword forbidden_vertices = 0;
                ADDELEMENT1(&forbidden_vertices, best_small_vertex);

                setword isolated_vertices = 0;
                construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                    forbidden_vertices, isolated_vertices, 0,
                                                                    max_colours_nbh_colouring_new, best_small_vertex, 0, 1);

            } else {
                times_doesnt_contain_small_vertex[nv]++;

                if(contains_small_colour_dominated_vertex(&best_small_vertex)) {
                    times_contains_small_vertex_colour[nv]++;

                    if(nv >= LEVEL_LEAST_CHILDREN  && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
                       && num_small_vertices > 1) {
                        int i;
                        int least_num_generated_from_parent = INT_MAX;
                        for(i = 0; i < num_small_vertices && least_num_generated_from_parent > 0; i++) {
                            number_of_graphs_generated_from_parent = 0;
                            number_of_uncol_graphs_generated_from_parent = 0;

                            //It is known that there is no valid colouring with num_colours!
                            int max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(small_vertices[i], num_colours - 1);

                            current_set[0] = small_vertices[i];
                            setword forbidden_vertices = 0;
                            ADDELEMENT1(&forbidden_vertices, small_vertices[i]);

                            setword isolated_vertices = 0;
                            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                                forbidden_vertices, isolated_vertices, 0,
                                                                                max_colours_nbh_colouring_new, small_vertices[i], 0, 0);

                            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                                if(number_of_graphs_generated_from_parent == 0) {
                                    times_no_children_small_vert_col[nv]++;
                                    if(number_of_uncol_graphs_generated_from_parent == 0)
                                        return; //Can't become better than this!     
                                }
                                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                                best_small_vertex = small_vertices[i];
                            }
                            if(i == BREAK_VALUE_LEAST_CHILDREN)
                                break;
                        }
                        //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
                        if(least_num_generated_from_parent > 0)
                            times_children_small_vert_col[nv]++;
                    }

                    //It is known that there is no valid colouring with num_colours!
                    int max_colours_nbh_colouring_new = max_colours_in_valid_neighbourhood_colouring(best_small_vertex, num_colours - 1);

                    current_set[0] = best_small_vertex;
                    setword forbidden_vertices = 0;
                    ADDELEMENT1(&forbidden_vertices, best_small_vertex);

                    setword isolated_vertices = 0;
                    construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, forbidden_vertices, 0,
                                                                        forbidden_vertices, isolated_vertices, 0,
                                                                        max_colours_nbh_colouring_new, best_small_vertex, 0, 1);

                } else {
                    times_doesnt_contain_small_vertex_colour[nv]++;

                    unsigned char similar_edgepair[4];
                    if(contains_similar_edgepair(similar_edgepair, 0)) {
                        times_contains_similar_edgepair[nv]++;

                        if(nv >= LEVEL_LEAST_CHILDREN  && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
                           && num_similar_edges > 1) {
                            int i;
                            int least_num_generated_from_parent = INT_MAX;
                            int best_sim_pair = 0;
                            for(i = 0; i < num_similar_edges && least_num_generated_from_parent > 0; i++) {
                                number_of_graphs_generated_from_parent = 0;
                                number_of_uncol_graphs_generated_from_parent = 0;

                                similar_edgepair[0] = similar_edges[i][0];
                                similar_edgepair[1] = similar_edges[i][1];
                                similar_edgepair[2] = similar_edges[i][2];
                                similar_edgepair[3] = similar_edges[i][3];

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
                                construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0, forbidden_vertices,
                                                                                    isolated_vertices, similar_edgepair[2], 0, 0, 0, 0);

                                //Generate sets which are adjacent to similar_edgepair[1] but not to similar_edgepair[3]
                                current_set[0] = similar_edgepair[1];
                                current_set_bitvector = 0;
                                ADDELEMENT1(&current_set_bitvector, current_set[0]);

                                forbidden_vertices = 0;
                                ADDELEMENT1(&forbidden_vertices, similar_edgepair[1]);
                                ADDELEMENT1(&forbidden_vertices, similar_edgepair[3]);
                                construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0, forbidden_vertices,
                                                                                    isolated_vertices, similar_edgepair[3], 0, 0, 0, 0);

                                if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                                    if(number_of_graphs_generated_from_parent == 0) {
                                        times_no_children_sim_edge[nv]++;
                                        if(number_of_uncol_graphs_generated_from_parent == 0)
                                            return; //Can't become better than this!
                                    }
                                    least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                                    best_sim_pair = i;
                                }
                                if(i == BREAK_VALUE_LEAST_CHILDREN)
                                    break;
                            }
                            similar_edgepair[0] = similar_edges[best_sim_pair][0];
                            similar_edgepair[1] = similar_edges[best_sim_pair][1];
                            similar_edgepair[2] = similar_edges[best_sim_pair][2];
                            similar_edgepair[3] = similar_edges[best_sim_pair][3];
                            //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);                    
                            if(least_num_generated_from_parent > 0)
                                times_children_sim_edge[nv]++;
                        }

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
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0, forbidden_vertices,
                                                                            isolated_vertices, similar_edgepair[2], 0, 0, 0, 1);

                        //Generate sets which are adjacent to similar_edgepair[1] but not to similar_edgepair[3]
                        current_set[0] = similar_edgepair[1];
                        current_set_bitvector = 0;
                        ADDELEMENT1(&current_set_bitvector, current_set[0]);
                        forbidden_vertices = 0;
                        ADDELEMENT1(&forbidden_vertices, similar_edgepair[1]);
                        ADDELEMENT1(&forbidden_vertices, similar_edgepair[3]);
                        construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0, forbidden_vertices,
                                                                            isolated_vertices, similar_edgepair[3], 0, 0, 0, 1);

                        //TODO: duplicated sets will be generated in case they contain both 
                        //similar_edgepair[0] and similar_edgepair[1]
                        //But probably this won't matter that much?

                        //VERY IMPORTANT:
                        //Must perform both expansions like this (each time with a different avoided vertex)
                        //because similar_element_is_destroyed() relies that both will be performed
                        //or else not all graphs will be generated!

                        //Total sets at the moment: 2^(n-1)

                        //fprintf(stderr, "nv: %d number_of_vertex_sets_global: %d\n", nv, number_of_vertex_sets_global);
                    } else {
                        times_doesnt_contain_similar_edgepair[nv]++;

                        compute_colour_domination_graphs_edge_removed();
                        if(contains_similar_edgepair(similar_edgepair, 1)) {
                            times_contains_similar_edgepair_col_dom[nv]++;

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
                            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0,
                                                                                forbidden_vertices, isolated_vertices, similar_edgepair[2], 0, 0, 0, 1);

                            //Generate sets which are adjacent to similar_edgepair[1] but not to similar_edgepair[3]
                            current_set[0] = similar_edgepair[1];
                            current_set_bitvector = 0;
                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                            ADDELEMENT1(&forbidden_vertices, similar_edgepair[1]);
                            ADDELEMENT1(&forbidden_vertices, similar_edgepair[3]);
                            construct_and_expand_vertex_sets_forbidden_vertices(current_set, 1, current_set_bitvector, 0,
                                                                                forbidden_vertices, isolated_vertices, similar_edgepair[3], 0, 0, 0, 1);
                        } else {
                            times_doesnt_contain_similar_edgepair_col_dom[nv]++;

                            unsigned char similar_trianglepair[6];
                            if(!test_if_trianglefree && contains_similar_triangle(similar_trianglepair, 0)) {
                                times_contains_similar_triangle[nv]++;

                                setword isolated_vertices = 0;
                                ADDELEMENT1(&isolated_vertices, similar_trianglepair[0]);
                                ADDELEMENT1(&isolated_vertices, similar_trianglepair[1]);
                                ADDELEMENT1(&isolated_vertices, similar_trianglepair[2]);

                                //Generate sets which are adjacent to similar_trianglepair[0] but not to similar_trianglepair[3]
                                current_set[0] = similar_trianglepair[0];
                                setword current_set_bitvector = 0;
                                ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                setword forbidden_vertices = 0;
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[0]);
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[3]);
                                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                    1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[3], 0, 0, 0, 1);

                                //Generate sets which are adjacent to similar_trianglepair[1] but not to similar_trianglepair[4]
                                current_set[0] = similar_trianglepair[1];
                                current_set_bitvector = 0;
                                ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                forbidden_vertices = 0;
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[1]);
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[4]);
                                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                    1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[4], 0, 0, 0, 1);

                                //Generate sets which are adjacent to similar_trianglepair[2] but not to similar_trianglepair[5]
                                current_set[0] = similar_trianglepair[2];
                                current_set_bitvector = 0;
                                ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                forbidden_vertices = 0;
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[2]);
                                ADDELEMENT1(&forbidden_vertices, similar_trianglepair[5]);
                                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                    1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[5], 0, 0, 0, 1);

                                //TODO: duplicated sets will be generated in case they contain both 
                                //similar_edgepair[0] and similar_edgepair[1]
                                //But probably this won't matter that much?

                                //Total sets at the moment: 2*2^(n-2) + 2^(n-2) = 2^(n-1) + 2^(n-2)
                            } else {
                                times_doesnt_contain_similar_triangle[nv]++;

                                if(!test_if_trianglefree)
                                    compute_colour_domination_graphs_triangle_removed();
                                if(!test_if_trianglefree &&
                                   contains_similar_triangle(similar_trianglepair, 1)) {
                                    times_contains_similar_triangle_col_dom[nv]++;

                                    setword isolated_vertices = 0;
                                    ADDELEMENT1(&isolated_vertices, similar_trianglepair[0]);
                                    ADDELEMENT1(&isolated_vertices, similar_trianglepair[1]);
                                    ADDELEMENT1(&isolated_vertices, similar_trianglepair[2]);

                                    //Generate sets which are adjacent to similar_trianglepair[0] but not to similar_trianglepair[3]
                                    current_set[0] = similar_trianglepair[0];
                                    setword current_set_bitvector = 0;
                                    ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                    setword forbidden_vertices = 0;
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[0]);
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[3]);
                                    construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                        1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[3], 0, 0, 0, 1);

                                    //Generate sets which are adjacent to similar_trianglepair[1] but not to similar_trianglepair[4]
                                    current_set[0] = similar_trianglepair[1];
                                    forbidden_vertices = 0;
                                    current_set_bitvector = 0;
                                    ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[1]);
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[4]);
                                    construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                        1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[4], 0, 0, 0, 1);

                                    //Generate sets which are adjacent to similar_trianglepair[2] but not to similar_trianglepair[5]
                                    current_set[0] = similar_trianglepair[2];
                                    current_set_bitvector = 0;
                                    ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                    forbidden_vertices = 0;
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[2]);
                                    ADDELEMENT1(&forbidden_vertices, similar_trianglepair[5]);
                                    construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                        1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_trianglepair[5], 0, 0, 0, 1);

                                    //Remark: duplicated sets will be generated in case they contain both 
                                    //similar_edgepair[0] and similar_edgepair[1]
                                    //But probably this won't matter that much?

                                    //Total sets at the moment: 2*2^(n-2) + 2^(n-2) = 2^(n-1) + 2^(n-2)
                                } else {
                                    times_doesnt_contain_similar_triangle_col_dom[nv]++;

                                    //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
                                    unsigned char similar_P4pair[8];

                                    if(contains_similar_P4(similar_P4pair, 0)) {
                                        times_contains_similar_P4[nv]++;

                                        setword isolated_vertices = 0;
                                        ADDELEMENT1(&isolated_vertices, similar_P4pair[0]);
                                        ADDELEMENT1(&isolated_vertices, similar_P4pair[1]);
                                        ADDELEMENT1(&isolated_vertices, similar_P4pair[2]);
                                        ADDELEMENT1(&isolated_vertices, similar_P4pair[3]);

                                        //Generate sets which are adjacent to similar_P4pair[0] but not to similar_P4pair[4]
                                        current_set[0] = similar_P4pair[0];
                                        setword current_set_bitvector = 0;
                                        ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                        setword forbidden_vertices = 0;
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[0]);
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[4]);
                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                            1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P4pair[4], 0, 0, 0, 1);

                                        //Generate sets which are adjacent to similar_P4pair[1] but not to similar_P4pair[5]
                                        current_set[0] = similar_P4pair[1];
                                        current_set_bitvector = 0;
                                        ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                        forbidden_vertices = 0;
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[1]);
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[5]);
                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                            1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P4pair[5], 0, 0, 0, 1);

                                        //Generate sets which are adjacent to similar_P4pair[2] but not to similar_P4pair[6]
                                        current_set[0] = similar_P4pair[2];
                                        current_set_bitvector = 0;
                                        ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                        forbidden_vertices = 0;
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[2]);
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[6]);
                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                            1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P4pair[6], 0, 0, 0, 1);

                                        //Generate sets which are adjacent to similar_P4pair[3] but not to similar_P4pair[7]
                                        current_set[0] = similar_P4pair[3];
                                        current_set_bitvector = 0;
                                        ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                        forbidden_vertices = 0;
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[3]);
                                        ADDELEMENT1(&forbidden_vertices, similar_P4pair[7]);
                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                            1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P4pair[7], 0, 0, 0, 1);

                                    } else {
                                        times_doesnt_contain_similar_P4[nv]++;

                                        //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
                                        unsigned char similar_diamondpair[8];
                                        if(!test_if_trianglefree && !test_if_diamondfree
                                           && contains_similar_diamond(similar_diamondpair, 0)) {
                                            times_contains_similar_diamond[nv]++;

                                            setword isolated_vertices = 0;
                                            ADDELEMENT1(&isolated_vertices, similar_diamondpair[0]);
                                            ADDELEMENT1(&isolated_vertices, similar_diamondpair[1]);
                                            ADDELEMENT1(&isolated_vertices, similar_diamondpair[2]);
                                            ADDELEMENT1(&isolated_vertices, similar_diamondpair[3]);

                                            //Generate sets which are adjacent to similar_diamondpair[0] but not to similar_diamondpair[4]
                                            current_set[0] = similar_diamondpair[0];
                                            setword current_set_bitvector = 0;
                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                            setword forbidden_vertices = 0;
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[0]);
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[4]);
                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_diamondpair[4], 0, 0, 0, 1);

                                            //Generate sets which are adjacent to similar_diamondpair[1] but not to similar_diamondpair[5]
                                            current_set[0] = similar_diamondpair[1];
                                            current_set_bitvector = 0;
                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                            forbidden_vertices = 0;
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[1]);
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[5]);
                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_diamondpair[5], 0, 0, 0, 1);

                                            //Generate sets which are adjacent to similar_diamondpair[2] but not to similar_diamondpair[6]
                                            current_set[0] = similar_diamondpair[2];
                                            current_set_bitvector = 0;
                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                            forbidden_vertices = 0;
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[2]);
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[6]);
                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_diamondpair[6], 0, 0, 0, 1);

                                            //Generate sets which are adjacent to similar_diamondpair[3] but not to similar_diamondpair[7]
                                            current_set[0] = similar_diamondpair[3];
                                            current_set_bitvector = 0;
                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                            forbidden_vertices = 0;
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[3]);
                                            ADDELEMENT1(&forbidden_vertices, similar_diamondpair[7]);
                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_diamondpair[7], 0, 0, 0, 1);

                                        } else {
                                            times_doesnt_contain_similar_diamond[nv]++;

                                            //Crown lemmas only work in case of 3 colours!
                                            setword crown_vertices;
                                            if(num_colours == 3 && contains_odd_crown(&crown_vertices)) {
                                                times_contains_odd_crown[nv]++;

                                                if(nv >= LEVEL_LEAST_CHILDREN && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
                                                   && num_cycles_stored > 1) {
                                                    int i;
                                                    int least_num_generated_from_parent = INT_MAX;
                                                    for(i = 0; i < num_cycles_stored && least_num_generated_from_parent > 0; i++) {
                                                        number_of_graphs_generated_from_parent = 0;
                                                        number_of_uncol_graphs_generated_from_parent = 0;

                                                        setword isolated_vertices = 0;
                                                        setword forbidden_vertices = 0;
                                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                            0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 0);

                                                        if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                                                            if(number_of_graphs_generated_from_parent == 0) {
                                                                times_no_children_odd_crown[nv]++;
                                                                if(number_of_uncol_graphs_generated_from_parent == 0)
                                                                    return; //Can't become better than this!
                                                            }

                                                            least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                                                            crown_vertices = stored_cycles_bitvectors[i];
                                                        }
                                                        if(i == BREAK_VALUE_LEAST_CHILDREN)
                                                            break;
                                                    }
                                                    //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
                                                    if(least_num_generated_from_parent > 0)
                                                        times_children_odd_crown[nv]++;
                                                }

                                                //Destroy even crown by applying all possible expansions
                                                //which contain at least one of the vertices of crown_vertices

                                                setword isolated_vertices = 0;
                                                setword forbidden_vertices = 0;
                                                construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                    0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, crown_vertices, 1);

                                            } else {
                                                times_doesnt_contain_odd_crown[nv]++;

                                                if(num_colours == 3 && contains_even_crown(&crown_vertices)) {
                                                    times_contains_even_crown[nv]++;

                                                    if(nv >= LEVEL_LEAST_CHILDREN && (nv % LEAST_CHILDREN_LEVEL_MOD) == 0
                                                       && num_cycles_stored > 1) {
                                                        int i;
                                                        int least_num_generated_from_parent = INT_MAX;
                                                        for(i = 0; i < num_cycles_stored && least_num_generated_from_parent > 0; i++) {
                                                            number_of_graphs_generated_from_parent = 0;
                                                            number_of_uncol_graphs_generated_from_parent = 0;

                                                            setword isolated_vertices = 0;
                                                            setword forbidden_vertices = 0;
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, stored_cycles_bitvectors[i], 0);

                                                            if(number_of_graphs_generated_from_parent < least_num_generated_from_parent) {
                                                                if(number_of_graphs_generated_from_parent == 0) {
                                                                    times_no_children_even_crown[nv]++;
                                                                    if(number_of_uncol_graphs_generated_from_parent == 0)
                                                                        return; //Can't become better than this!
                                                                }

                                                                least_num_generated_from_parent = number_of_graphs_generated_from_parent;
                                                                crown_vertices = stored_cycles_bitvectors[i];
                                                            }
                                                            if(i == BREAK_VALUE_LEAST_CHILDREN)
                                                                break;
                                                        }
                                                        //fprintf(stderr, "nv=%d, least num generated: %d\n", nv, least_num_generated_from_parent);
                                                        if(least_num_generated_from_parent > 0)
                                                            times_children_even_crown[nv]++;
                                                    }

                                                    //Destroy even crown by applying all possible expansions
                                                    //which contain at least one of the vertices of crown_vertices

                                                    setword isolated_vertices = 0;
                                                    setword forbidden_vertices = 0;
                                                    construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                        0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, crown_vertices, 1);


                                                } else {
                                                    times_doesnt_contain_even_crown[nv]++;

                                                    unsigned char best_cutvertex;
                                                    int number_of_components;
                                                    setword best_comp;
                                                    if(contains_cutvertex(&best_cutvertex, &number_of_components, &best_comp)) {
                                                        times_contains_cutvertex[nv]++;

                                                        //fprintf(stderr, "number_of_components: %d\n", number_of_components);

                                                        setword isolated_vertices = 0;
                                                        setword forbidden_vertices = 0;

                                                        //setword forced_vertices_at_least_one = ALLMASK(nv);
                                                        //DELELEMENT1(&forced_vertices_at_least_one, best_cutvertex);

                                                        //fprintf(stderr, "nv=%d, num vertices in best comp: %d\n", nv, POPCOUNT(best_comp));

                                                        setword forced_vertices_at_least_one = best_comp;

                                                        //TODO: eigenlijk moet buur hebben in 1 van de comp en kan kiezen welke!

                                                        construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                            0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, forced_vertices_at_least_one, 1);

                                                    } else {
                                                        times_doesnt_contain_cutvertex[nv]++;

                                                        //Don't perform colour test since that doesn't fit into memory (+ doesn't help much anyway)
                                                        unsigned char similar_P6pair[12];

                                                        //If max_path_size < 6, then the graph will be P6-free!
                                                        if(max_path_size >= 6 && contains_similar_P6(similar_P6pair, 0)) {
                                                            times_contains_similar_P6[nv]++;

                                                            setword isolated_vertices = 0;
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[0]);
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[1]);
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[2]);
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[3]);
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[4]);
                                                            ADDELEMENT1(&isolated_vertices, similar_P6pair[5]);

                                                            //Generate sets which are adjacent to similar_P6pair[0] but not to similar_P6pair[6]
                                                            current_set[0] = similar_P6pair[0];
                                                            setword current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            setword forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[0]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[6]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[6], 0, 0, 0, 1);

                                                            //Generate sets which are adjacent to similar_P6pair[1] but not to similar_P6pair[7]
                                                            current_set[0] = similar_P6pair[1];
                                                            current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[1]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[7]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[7], 0, 0, 0, 1);

                                                            //Generate sets which are adjacent to similar_P6pair[2] but not to similar_P6pair[8]
                                                            current_set[0] = similar_P6pair[2];
                                                            current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[2]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[8]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[8], 0, 0, 0, 1);

                                                            //Generate sets which are adjacent to similar_P6pair[3] but not to similar_P6pair[9]
                                                            current_set[0] = similar_P6pair[3];
                                                            current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[3]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[9]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[9], 0, 0, 0, 1);

                                                            //Generate sets which are adjacent to similar_P6pair[4] but not to similar_P6pair[10]
                                                            current_set[0] = similar_P6pair[4];
                                                            current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[4]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[10]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[10], 0, 0, 0, 1);

                                                            //Generate sets which are adjacent to similar_P6pair[5] but not to similar_P6pair[11]
                                                            current_set[0] = similar_P6pair[5];
                                                            current_set_bitvector = 0;
                                                            ADDELEMENT1(&current_set_bitvector, current_set[0]);
                                                            forbidden_vertices = 0;
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[5]);
                                                            ADDELEMENT1(&forbidden_vertices, similar_P6pair[11]);
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                1, current_set_bitvector, 0, forbidden_vertices, isolated_vertices, similar_P6pair[11], 0, 0, 0, 1);

                                                        } else {
                                                            times_doesnt_contain_similar_P6[nv]++;


                                                            times_no_lemmas_applicable[nv]++;

                                                            //if(nv == maxnv - 1) {
                                                            //if(nv >= 13) {
                                                            //  fprintf(stderr, "Info: outputting graph where no lemmas applicable\n");
                                                            //output_graph(current_graph, adj, nv);
                                                            //}

                                                            setword isolated_vertices = 0;

                                                            //Just use the same method with no forbidden vertices
                                                            //Total sets: 2^n-1
                                                            setword forbidden_vertices = 0;
                                                            construct_and_expand_vertex_sets_forbidden_vertices(current_set,
                                                                                                                0, 0, 0, forbidden_vertices, isolated_vertices, 0, 0, 0, 0, 1);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

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

//Makes a cycle of a given size
static void
make_complete_graph(int size) {
    nv = size;
    int i, j;
    for(i = 0; i < size; i++) {
        adj[i] = 0;
        for(j = 0; j < size; j++)
            if(j != i)
                current_graph[i][adj[i]++] = j;
    }
}

/******************************************************************************/

//Makes a cycle of a given size
static void
make_cycle(int cycle_size) {
    nv = cycle_size;
    int i;
    for(i = 0; i < cycle_size; i++) {
        adj[i] = 2;
        current_graph[i][0] = (i + cycle_size - 1) % cycle_size;
        current_graph[i][1] = (i + 1) % cycle_size;
    }
}


/******************************************************************************/

//Makes an antihole of a given size
static void
make_antihole(int size) {
    nv = size;
    int i, j;
    for(i = 0; i < size; i++) {
        adj[i] = 0;
        for(j = 0; j < size; j++)
            if(j != i && j != ((i + size - 1) % size)
               && j != ((i + 1) % size))
                current_graph[i][adj[i]++] = j;
    }
}

/**************************************************************************/

static void
add_edge(GRAPH current_graph, unsigned char adj[], unsigned char v, unsigned char w) {
    current_graph[v][adj[v]] = w;
    current_graph[w][adj[w]] = v;
    adj[v]++;
    adj[w]++;
}

/**************************************************************************/

/**
 * Decodes the code (which is in multicode format) of a graph.
 */
static void
decode_multicode(unsigned char code[], GRAPH current_graph, ADJACENCY adj, int codelength) {
    int i;

    if(nv != code[0]) {
        fprintf(stderr, "Error: Wrong number of vertices: expected %d while found %d vertices \n", nv, code[0]);
        exit(1);
    }

    for(i = 0; i < nv; i++) {
        adj[i] = 0;
        //neighbours[i] = 0;
    }

    int currentvertex = 1;
    for(i = 1; i < codelength; i++) {
        if(code[i] == 0) {
            currentvertex++;
        } else {
            add_edge(current_graph, adj, currentvertex - 1, code[i] - 1);
        }
    }

    for(i = 0; i < nv; i++) {
        if(adj[i] > MAXN - 1) {
            fprintf(stderr, "Error: graph can have at most %d neighbours, but found %d neighbours\n",
                    MAXN - 1, adj[i]);
            exit(1);
        }
    }

}

/******************************************************************************/

static void
init_construction() {
    //options.userautomproc = save_generators;
    options.defaultptn = TRUE;
    //options.getcanon = FALSE;
    options.getcanon = TRUE;

    nautyg_colour_dominated_edge_removed = malloc(sizeof(graph) * MAXN * MAXN * MAXN * MAXN);
    if(nautyg_colour_dominated_edge_removed == NULL) {
        fprintf(stderr, "Error: Can't get enough memory while creating nautyg_colour_dominated_edge_removed\n");
        exit(1);
    }

    nautyg_colour_dominated_triangle_removed = malloc(sizeof(graph) * MAXN * MAXN * MAXN * MAXN * MAXN);
    if(nautyg_colour_dominated_triangle_removed == NULL) {
        fprintf(stderr, "Error: Can't get enough memory while creating nautyg_colour_dominated_triangle_removed\n");
        exit(1);
    }

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

    //Limit startgraphs based on strong perfect graph theorem
    setword isolated_vertices = 0;

    if(start_from_inputgraphs) {
        unsigned char code[MAXN * (MAXN-1) / 2 + MAXN];
        unsigned long long int total_number_of_inputgraphs_read = 0;
        int nuller;
        int codelength;
        int nv_previous = -1;

        while(fread(&nv, sizeof (unsigned char), 1, stdin)) {
            if(nv > MAXN) {
                fprintf(stderr, "Error: can only read graphs with at most %d vertices (tried to read graph with %d vertices)\n", MAXN, nv);
                exit(1);
            }

            if(nv > maxnv) {
                fprintf(stderr, "Error: inputgraph is too big: %d vs max %d\n", nv, maxnv);
                exit(1);
            }

            if(modulo > 1 && nv > splitlevel) {
                fprintf(stderr, "Error: graph is larger than splitlevel!\n");
                exit(1);
            }

            if(nv_previous != -1 && nv != nv_previous) {
                fprintf(stderr, "Error: read inputgraphs with different nv!\n");
                exit(1);
            }
            nv_previous = nv;

            code[0] = nv;
            nuller = 0;
            codelength = 1;
            while(nuller < nv - 1) {
                code[codelength] = getc(stdin);
                if(code[codelength] == 0) nuller++;
                codelength++;
            }
            decode_multicode(code, current_graph, adj, codelength);

            total_number_of_inputgraphs_read++;

            fprintf(stderr, "Processing graph %llu with %d vertices\n", total_number_of_inputgraphs_read, nv);

            copy_nauty_graph(current_graph, adj, nv, nautyg);

            //printgraph(current_graph, adj, nv);
            //printgraph_nauty(nautyg, nv);

            extend(isolated_vertices, 0, 0, 0, 1);

        }

        fprintf(stderr, "Read %llu inputgraphs\n", total_number_of_inputgraphs_read);

        return;
    }

#ifdef LIMIT_STARTGRAPHS
    //3 colours -> output K4
    //4 colours -> output K5
    if(!test_if_trianglefree && !test_if_K4free && !(test_if_K5free && num_colours >= 4)) {
        make_complete_graph(num_colours + 1);
        copy_nauty_graph(current_graph, adj, nv, nautyg); //necessary for planarity test
        if(!test_if_planar || is_planar()) {
            fprintf(stderr, "Info: writing K%d\n", num_colours + 1);
            output_graph(current_graph, adj, nv);
        }
    }

    int max_cycle_size;
    if(!test_for_disjoint_paths && !test_if_contains_induced_fork) {
        //C_(k+1) contains induced P_k
        //So in case of P_k free, longest allowed induced cycle is C_k
        //P6-free is max_path_size == 5
        max_cycle_size = max_path_size + 1;
    } else
        max_cycle_size = maxnv;

    if((max_cycle_size % 2) == 0)
        max_cycle_size--;

    if(max_cycle_size > maxnv) {
        fprintf(stderr, "Error: max cycle size cannot be larger than maxnv!\n");
        exit(1);
    }

    //int i;
    //Starting from large or small hardly makes any difference...
    for(i = 5; i <= max_cycle_size; i += 2)
        //for(i = max_cycle_size; i >= 5; i -= 2)
        if(!(test_if_contains_forbidden_cycle && forbidden_cycle_size == i)) {
            make_cycle(i);

            //Only go above splitlevel if splitcounter == rest (is better than just case 0)
            if(modulo == 1 || (splitcounter == rest || nv <= splitlevel)) {
                fprintf(stderr, "Info: starting construction from C%d\n", i);

                copy_nauty_graph(current_graph, adj, nv, nautyg);

                //printgraph(current_graph, adj, nv);
                //printgraph_nauty(nautyg, nv);

                extend(isolated_vertices, 0, 0, 0, 1);
            } else
                fprintf(stderr, "Info: NOT starting construction from C%d since only doing this in case %d\n", i, splitcounter);

        }

    if(!test_if_trianglefree) {
        //3 colours: Only antihole C7 since C9 contains a K4
        //4 colours: Antihole C7 and antihole C9 since antihole C11 contains a K5

        //Clique number of Antihole C 2k+1 is k
        //So a larger antihole cannot be a valid startgraph as it contains a K_{k+1}
        int largest_antihole_size = 2 * num_colours + 1;

        //TODO: check that not larger than nv or splitlevel (if used)!
        for(i = 7; i <= largest_antihole_size; i += 2) {
            fprintf(stderr, "Info: starting construction from antihole C%d (chromatic number: %d)\n",
                    i, (i - 1) / 2 + 1);
            make_antihole(i);
            copy_nauty_graph(current_graph, adj, nv, nautyg);

            //printgraph(current_graph, adj, nv);
            //printgraph_nauty(nautyg, nv);

            if(!(test_if_contains_forbidden_cycle
                 && contains_forbidden_induced_cycle(forbidden_cycle_size)))
                extend(isolated_vertices, 0, 0, 0, 1);
            else
                fprintf(stderr, "Info: not extending antihole C%d as it contains an induced C%d\n",
                        i, forbidden_cycle_size);
        }
    } else
        fprintf(stderr, "Info: not extending antiholes as they all contain a triangle\n");
#else
    //Initialise construction from the isolated vertex
    nv = 1;
    adj[0] = 0;

    copy_nauty_graph(current_graph, adj, nv, nautyg);

    //printgraph(current_graph, adj, nv);
    //printgraph_nauty(nautyg, nv);

    extend(isolated_vertices, 0, 0, 0, 1);

#endif

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
    fprintf(stderr, "  diamondfree: Only output diamond-free graphs.\n");
    fprintf(stderr, "  Gem: Only output gem-free graphs.\n");
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

    int edge_critical_test_is_supported = 1;

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
            if(maxnv > MAXN) {
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
                    if(max_path_size == 0) {
                        //If Px forbidden, max nv in path is x-1
                        max_path_size = atoi(argv[i] + 1) - 1;
                        if(max_path_size < 1) {
                            fprintf(stderr, "Error: invalid filter value: %d\n", max_path_size);
                            exit(1);
                        }
                        fprintf(stderr, "Info: only outputting graphs with no induced P%d's\n", max_path_size + 1);
                    } else {
                        test_for_disjoint_paths = 1;
                        max_path_size_disjoint_smaller = atoi(argv[i] + 1) - 1;
                        if(max_path_size_disjoint_smaller > max_path_size) {
                            fprintf(stderr, "Error: second induced forbidden path cannot be larger than first one\n");
                            exit(1);
                        }
                        if(max_path_size_disjoint_smaller < 0) {
                            fprintf(stderr, "Error: invalid filter value: %d\n", max_path_size_disjoint_smaller);
                            exit(1);
                        }

                        edge_critical_test_is_supported = 0;
                    }
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
                case 'g':
                {
                    //Only accepts graphs which are NOT k-colourable (on last level)
                    girth = atoi(argv[i] + 1);
                    if(girth < 4) {
                        fprintf(stderr, "Error: when using girth option, it must be at least 4!\n");
                        exit(1);
                    }
                    if(girth > 7) {
                        fprintf(stderr, "Error: girth filter is not very efficient for large girths...\n");
                        fprintf(stderr, "Max girth is: 7\n");
                        exit(1);
                    }
                    test_girth = 1;
                    test_if_trianglefree = 1;
                    if(girth == 4)
                        test_girth = 0; //Do triangle-free instead

                    edge_critical_test_is_supported = 0;

                    fprintf(stderr, "Info: only outputting graphs with girth >= %d\n", girth);
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
                        fprintf(stderr, "Warning: tests for induced diamonds. This is quite slow. If you know the graphs will be K4-free, you don't need to test for induced diamonds (much faster).\n");

                        edge_critical_test_is_supported = 0;
                    }
                    break;
                }
                case 'K':
                {
                    if(strcmp(argv[i], "K4") == 0) {
                        test_if_K4free = 1;
                        fprintf(stderr, "Info: only generating graphs which are K4-free\n");
                        edge_critical_test_is_supported = 0;
                    } else if(strcmp(argv[i], "K5") == 0) {
                        test_if_K5free = 1;
                        fprintf(stderr, "Info: only generating graphs which are K5-free\n");
                        edge_critical_test_is_supported = 0;
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
                case 'p':
                {
                    if(strcmp(argv[i], "planar") == 0) {
                        test_if_planar = 1;
                        fprintf(stderr, "Info: only generating graphs which are planar\n");
                    }
                    break;
                }
                case 'i':
                {
                    if(strcmp(argv[i], "input") == 0) {
                        start_from_inputgraphs = 1;
                        fprintf(stderr, "Info: only starting from inputgraphs\n");
                    }
                    break;
                }
                case 'f':
                {
                    if(strcmp(argv[i], "fork") == 0) {
                        test_if_contains_induced_fork = 1;
                        edge_critical_test_is_supported = 0;
                    }
                    break;
                }
                case 'b':
                {
                    if(strcmp(argv[i], "bull") == 0) {
                        test_if_contains_induced_bull = 1;
                        edge_critical_test_is_supported = 0;
                    }
                    break;
                }
                case 'G':
                {
                    if(strcmp(argv[i], "Gem") == 0) {
                        test_if_contains_induced_gem = 1;
                        fprintf(stderr, "Info: searching for gem-free graphs\n");
                        edge_critical_test_is_supported = 0;
                    }
                    break;
                }

                // MA. "UI" functionality from adding is_incuded property
                // Lowercase i is taken
                case 'I':
                    {
                        // strcmp compares two character strings
                        if(strcmp(argv[i], "2") == 0) {
                            test_if_contains_induced_copy = 1;
                            fprintf(stderr, "Info: Querying if graph contains this induced copy (indueced copy here)\n");
                            edge_critical_test_is_supported = 0;
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

    if(test_if_contains_forbidden_cycle)
        fprintf(stderr, "Program for outputting %d-critical (P%d,C%d)-free graphs\n", num_colours + 1, max_path_size + 1, forbidden_cycle_size);
    else
        fprintf(stderr, "Program for outputting %d-critical P%d-free graphs\n", num_colours + 1, max_path_size + 1);

    if(test_for_disjoint_paths)
        fprintf(stderr, "Actually only outputting (P%d+P%d)-free graphs!\n", max_path_size + 1, max_path_size_disjoint_smaller + 1);

#ifdef APPLY_EXTRA_P1_FREE_TEST
    if(test_for_disjoint_paths && max_path_size_disjoint_smaller == 0)
        fprintf(stderr, "Also applying extra P1-free test\n");
#endif


    if(test_if_contains_induced_fork) {
        fprintf(stderr, "Warning: Only outputting (C3,fork)-free graphs and not searching for Pt-freeness!\n");

        if(max_path_size != 0) {
            fprintf(stderr, "Error: not testing for induced paths when looking for fork-free graphs!\n");
            exit(1);
        }

        max_path_size = 10; //So it can still search for crowns...

        fprintf(stderr, "Warning: only searching for triangle-free graphs!\n");
        test_if_trianglefree = 1;

    }

    if(test_if_contains_induced_bull) {
        fprintf(stderr, "Info: only searching for bull-free graphs\n");
    }

    if(test_if_contains_forbidden_cycle && test_girth) {
        fprintf(stderr, "Error: are you sure you want to test both forbidden cycles and girth?\n");
        exit(1);
    }

    fprintf(stderr, "Info: NUM_PREV_SETS: %d\n", NUM_PREV_SETS);
    fprintf(stderr, "Info: LEVEL_LEAST_CHILDREN: %d\n", LEVEL_LEAST_CHILDREN);
    if(LEVEL_LEAST_CHILDREN < MAXN)
        fprintf(stderr, "Info: Only applying least children trick if nv mod %d == 0\n", LEAST_CHILDREN_LEVEL_MOD);

#ifdef TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN
    fprintf(stderr, "Info: first testing if there is an expansion without children (from %d vertices)\n", CONTAINS_EXPANSION_WITHOUT_CHILDREN_LEVEL + 1);
#endif

    if(output_vertex_critical_graphs)
        fprintf(stderr, "Info: only outputting k-vertex-critical graphs\n");
    else {
#ifdef TEST_IF_CRITICAL
        fprintf(stderr, "Info: only outputting k-critical graphs\n");
        if(!edge_critical_test_is_supported) {
            fprintf(stderr, "Error: the edge-critical test is not supported with these parameters. Use the option 'vertexcritical' instead.\n");
            exit(1);
        }

#else
        fprintf(stderr, "Info: outputting possibly k-critical graphs (so you still need to perform a criticality test later)\n");
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

    if(test_if_diamondfree && !test_if_K4free) {
        //Diamondfree test is as subgraph, not as induced subgraph
        //But if it contains a diamond which is in a K4 this is also rejected in case of 
        //3 colours since graphs which contain a K4 (with nv > 4) cannot be 4-critical
        //fprintf(stderr, "Error: diamond-free test only works for 3 colours at the moment\n");
        //exit(1);
    }


    //For the timing
    struct tms TMS;

    int i;
    for(i = 0; i < argc; i++)
        fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");

    //Start construction
    initialize_splitting();

    init_construction();

    free(nautyg_colour_dominated_edge_removed);
    free(nautyg_colour_dominated_triangle_removed);
    free(previous_induced_paths);

    times(&TMS);
    unsigned int savetime = (unsigned int) TMS.tms_utime;

/*
    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim vertices: %llu (%.2f%%)\n", i, times_contains_similar_vertices[i],
            (double) times_contains_similar_vertices[i] / (times_contains_similar_vertices[i] + times_doesnt_contain_similar_vertices[i]) * 100);

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim vertices col_dom: %llu (%.2f%%)\n", i, times_contains_similar_vertices_col_dom[i],
            (double) times_contains_similar_vertices_col_dom[i] / (times_contains_similar_vertices_col_dom[i] + times_doesnt_contain_similar_vertices_col_dom[i]) * 100);

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim edgepair: %llu (%.2f%%)\n", i, times_contains_similar_edgepair[i],
            (double) times_contains_similar_edgepair[i] / (times_contains_similar_edgepair[i] + times_doesnt_contain_similar_edgepair[i]) * 100);

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains small vertex: %llu (%.2f%%)\n", i, times_contains_small_vertex[i],
            (double) times_contains_small_vertex[i] / (times_contains_small_vertex[i] + times_doesnt_contain_small_vertex[i]) * 100);

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains small vertex colour: %llu (%.2f%%)\n", i, times_contains_small_vertex_colour[i],
            (double) times_contains_small_vertex_colour[i] / (times_contains_small_vertex_colour[i] + times_doesnt_contain_small_vertex_colour[i]) * 100);
    
    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains sim edgepair col_dom: %llu (%.2f%%)\n", i, times_contains_similar_edgepair_col_dom[i],
            (double) times_contains_similar_edgepair_col_dom[i] / (times_contains_similar_edgepair_col_dom[i] + times_doesnt_contain_similar_edgepair_col_dom[i]) * 100);

    if(!test_if_trianglefree) {
        for(i = 10; i < maxnv; i++)
            fprintf(stderr, "Nv=%d, times contains sim triangle: %llu (%.2f%%)\n", i, times_contains_similar_triangle[i],
                (double) times_contains_similar_triangle[i] / (times_contains_similar_triangle[i] + times_doesnt_contain_similar_triangle[i]) * 100);

        for(i = 10; i < maxnv; i++)
            fprintf(stderr, "Nv=%d, times contains sim triangle col_dom: %llu (%.2f%%)\n", i, times_contains_similar_triangle_col_dom[i],
                (double) times_contains_similar_triangle_col_dom[i] / (times_contains_similar_triangle_col_dom[i] + times_doesnt_contain_similar_triangle_col_dom[i]) * 100);

        if(!test_if_diamondfree)
            for(i = 10; i < maxnv; i++)
                fprintf(stderr, "Nv=%d, times contains sim diamond: %llu (%.2f%%)\n", i, times_contains_similar_diamond[i],
                    (double) times_contains_similar_diamond[i] / (times_contains_similar_diamond[i] + times_doesnt_contain_similar_diamond[i]) * 100);
    }
*/

/*
        for(i = 10; i < maxnv; i++)
            fprintf(stderr, "Nv=%d, times contains sim P4: %llu (%.2f%%)\n", i, times_contains_similar_P4[i],
                (double) times_contains_similar_P4[i] / (times_contains_similar_P4[i] + times_doesnt_contain_similar_P4[i]) * 100);

        for(i = 10; i < maxnv; i++)
            fprintf(stderr, "Nv=%d, times contains sim P6: %llu (%.2f%%)\n", i, times_contains_similar_P6[i],
                (double) times_contains_similar_P6[i] / (times_contains_similar_P6[i] + times_doesnt_contain_similar_P6[i]) * 100);
*/


/*    
    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains even crown: %llu (%.2f%%)\n", i, times_contains_even_crown[i],
            (double) times_contains_even_crown[i] / (times_contains_even_crown[i] + times_doesnt_contain_even_crown[i]) * 100);

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains odd crown: %llu (%.2f%%)\n", i, times_contains_odd_crown[i],
            (double) times_contains_odd_crown[i] / (times_contains_odd_crown[i] + times_doesnt_contain_odd_crown[i]) * 100);    
    
    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times contains cutvertex: %llu (%.2f%%)\n", i, times_contains_cutvertex[i],
            (double) times_contains_cutvertex[i] / (times_contains_cutvertex[i] + times_doesnt_contain_cutvertex[i]) * 100);    
    
    for(i = 7; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times P%d found previous: %llu (%.2f%%)\n", i, max_path_size + 1, times_induced_path_found_previous[i],
                (double) times_induced_path_found_previous[i] / (times_induced_path_found_previous[i] + times_induced_path_not_found_previous[i]) * 100);                            
    
    for(i = 8; i <= maxnv; i++)
       fprintf(stderr, "Nv=%d, times P-free: %llu (%.2f%%)\n", i, times_graph_Pfree[i],
                (double) times_graph_Pfree[i] / (times_graph_Pfree[i] + times_graph_not_Pfree[i]) * 100);                

    if(test_if_contains_induced_fork)
        for(i = 8; i <= maxnv; i++)
           fprintf(stderr, "Nv=%d, times fork-free: %llu (%.2f%%)\n", i, times_graph_forkfree[i],
                    (double) times_graph_forkfree[i] / (times_graph_forkfree[i] + times_graph_not_forkfree[i]) * 100);                

    for(i = 9; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times colourable: %llu (%.2f%%)\n", i, times_graph_colourable[i],
                (double) times_graph_colourable[i] / (times_graph_colourable[i] + times_graph_not_colourable[i]) * 100);        
*/

    for(i = 8; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times P-free: %llu (%.2f%%)\n", i, times_graph_Pfree[i],
                (double) times_graph_Pfree[i] / (times_graph_Pfree[i] + times_graph_not_Pfree[i]) * 100);

    if(test_if_contains_induced_bull)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times bull-free: %llu (%.2f%%)\n", i, times_graph_bullfree[i],
                    (double) times_graph_bullfree[i] / (times_graph_bullfree[i] + times_graph_not_bullfree[i]) * 100);

    if(test_if_contains_induced_gem)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times gem-free: %llu (%.2f%%)\n", i, times_graph_gemfree[i],
                    (double) times_graph_gemfree[i] / (times_graph_gemfree[i] + times_graph_not_gemfree[i]) * 100);

    // MA. Print out of induced_copy results
    /*  
    if(test_if_contains_induced_copy)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times induced copy: %llu (%.2f%%)\n", i, times_graph_induced[i],
                    (double) times_graph_induced[i] / (times_graph_induced[i] + times_graph_not_induced[i]) * 100);
    */

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times no lemmas applicable: %llu\n", i, times_no_lemmas_applicable[i]);

    if(test_if_trianglefree)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times triangle-free: %llu (%.2f%%)\n",
                    i, counts_doesnt_contain_triangle[i],
                    (double) 100 * counts_doesnt_contain_triangle[i] / (counts_contains_triangle[i] + counts_doesnt_contain_triangle[i]));

    if(test_if_diamondfree)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times diamond-free: %llu (%.2f%%)\n",
                    i, counts_doesnt_contain_diamond[i],
                    (double) 100 * counts_doesnt_contain_diamond[i] / (counts_contains_diamond[i] + counts_doesnt_contain_diamond[i]));

/*
    if(test_if_K4free)
            for(i = 8; i <= maxnv; i++)
                fprintf(stderr, "Nv=%d, times contains K4: %llu (%.2f%%)\n",
                    i, counts_contains_K4[i],
                    (double) 100 * counts_contains_K4[i] / (counts_contains_K4[i] + counts_doesnt_contain_K4[i]));        
    
*/

    if(test_if_K5free)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times contains K5: %llu (%.2f%%)\n",
                    i, counts_contains_K5[i],
                    (double) 100 * counts_contains_K5[i] / (counts_contains_K5[i] + counts_doesnt_contain_K5[i]));

/*         
    if(!test_if_diamondfree && !test_if_trianglefree) {
        if(num_colours == 3) {
            for(i = 8; i <= maxnv; i++)
                fprintf(stderr, "Nv=%d, times contains K4: %llu (%.2f%%)\n",
                    i, counts_contains_K4[i],
                    (double) 100 * counts_contains_K4[i] / (counts_contains_K4[i] + counts_doesnt_contain_K4[i]));

            if(!(test_if_contains_forbidden_cycle && forbidden_cycle_size == 5))
                for(i = 8; i <= maxnv; i++)
                    fprintf(stderr, "Nv=%d, times doesn't contain K4 but contains W5 : %llu (%.2f%%)\n",
                        i, counts_contains_W5[i],
                        (double) 100 * counts_contains_W5[i] / (counts_contains_W5[i] + counts_doesnt_contain_W5[i]));
        } else if(num_colours == 4) {
            for(i = 8; i <= maxnv; i++)
                fprintf(stderr, "Nv=%d, times contains K5: %llu (%.2f%%)\n",
                    i, counts_contains_K5[i],
                    (double) 100 * counts_contains_K5[i] / (counts_contains_K5[i] + counts_doesnt_contain_K5[i]));
        }
    }


    if(test_if_contains_forbidden_cycle)
        for(i = 8; i <= maxnv; i++)
            fprintf(stderr, "Nv=%d, times C%d-free: %llu (%.2f%%)\n", i, forbidden_cycle_size, counts_doesnt_contain_forbidden_induced_cycle[i],
                    (double) counts_doesnt_contain_forbidden_induced_cycle[i] / (counts_doesnt_contain_forbidden_induced_cycle[i] + counts_contains_forbidden_induced_cycle[i]) * 100);    
    
    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times small vertex NOT destroyed: %llu (%.2f%%)\n", i, times_small_vertex_not_destroyed[i],
            (double) times_small_vertex_not_destroyed[i] / (times_small_vertex_not_destroyed[i] + times_small_vertex_destroyed[i]) * 100);    

    for(i = 10; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times similar element NOT destroyed: %llu (%.2f%%)\n", i, times_similar_element_not_destroyed[i],
                (double) times_similar_element_not_destroyed[i] / (times_similar_element_not_destroyed[i] + times_similar_element_destroyed[i]) * 100);            

    if(test_if_planar)
        for(i = 10; i < maxnv; i++)
            fprintf(stderr, "Nv=%d, times graph NOT planar: %llu (%.2f%%)\n", i, times_graph_isnt_planar[i],
                (double) times_graph_isnt_planar[i] / (times_graph_isnt_planar[i] + times_graph_is_planar[i]) * 100);                
    
    //for(i = 9; i < maxnv; i++)
    //    fprintf(stderr, "Nv=%d, times prev similar vertices no longer destroyed: %llu (%.2f%%)\n", i, times_similar_vertices_no_longer_destroyed[i],
    //            (double) times_similar_vertices_no_longer_destroyed[i] / (times_similar_vertices_no_longer_destroyed[i] + times_similar_vertices_are_still_destroyed[i]) * 100);            
    
    for(i = 7; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times P%d found previous: %llu (%.2f%%)\n", i, max_path_size + 1, times_induced_path_found_previous[i],
                (double) times_induced_path_found_previous[i] / (times_induced_path_found_previous[i] + times_induced_path_not_found_previous[i]) * 100);                        
    
    //for(i = 8; i <= maxnv; i++)
    //   fprintf(stderr, "Nv=%d, times P-free: %llu (%.2f%%)\n", i, times_graph_Pfree[i],
    //            (double) times_graph_Pfree[i] / (times_graph_Pfree[i] + times_graph_not_Pfree[i]) * 100);            
    
    for(i = 9; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times colourable: %llu (%.2f%%)\n", i, times_graph_colourable[i],
                (double) times_graph_colourable[i] / (times_graph_colourable[i] + times_graph_not_colourable[i]) * 100);        
    
    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children sim vert: %llu (%.2f%%)\n", i, times_no_children_sim_vert[i],
                (double) times_no_children_sim_vert[i] / (times_no_children_sim_vert[i] + times_children_sim_vert[i]) * 100);

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children sim vert col: %llu (%.2f%%)\n", i, times_no_children_sim_vert_col[i],
                (double) times_no_children_sim_vert_col[i] / (times_no_children_sim_vert_col[i] + times_children_sim_vert_col[i]) * 100);

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children small vert: %llu (%.2f%%)\n", i, times_no_children_small_vert[i],
                (double) times_no_children_small_vert[i] / (times_no_children_small_vert[i] + times_children_small_vert[i]) * 100);

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children small vert col: %llu (%.2f%%)\n", i, times_no_children_small_vert_col[i],
                (double) times_no_children_small_vert_col[i] / (times_no_children_small_vert_col[i] + times_children_small_vert_col[i]) * 100);

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children sim edge: %llu (%.2f%%)\n", i, times_no_children_sim_edge[i],
                (double) times_no_children_sim_edge[i] / (times_no_children_sim_edge[i] + times_children_sim_edge[i]) * 100);

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children sim edge col: %llu (%.2f%%)\n", i, times_no_children_sim_edge_col[i],
                (double) times_no_children_sim_edge_col[i] / (times_no_children_sim_edge_col[i] + times_children_sim_edge_col[i]) * 100);

#ifdef TEST_IF_CONTAINS_EXPANSION_WITHOUT_CHILDREN    
    for(i = 9; i < maxnv; i++)
        fprintf(stderr, "Nv=%d, times expansion with no children: %llu (%.2f%%)\n", i, times_expansion_with_no_children[i],
                (double) times_expansion_with_no_children[i] / (times_expansion_with_no_children[i] + times_no_expansion_with_no_children[i]) * 100);          
#endif    
*/

/*
    for(i = 7; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times P%d found previous: %llu (%.2f%%)\n", i, max_path_size + 1, times_induced_path_found_previous[i],
                (double) times_induced_path_found_previous[i] / (times_induced_path_found_previous[i] + times_induced_path_not_found_previous[i]) * 100);                        
    
    for(i = 8; i <= maxnv; i++)
       fprintf(stderr, "Nv=%d, times P-free: %llu (%.2f%%)\n", i, times_graph_Pfree[i],
                (double) times_graph_Pfree[i] / (times_graph_Pfree[i] + times_graph_not_Pfree[i]) * 100);         
*/

/*
    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children odd crown: %llu (%.2f%%)\n", i, times_no_children_odd_crown[i],
                (double) times_no_children_odd_crown[i] / (times_no_children_odd_crown[i] + times_children_odd_crown[i]) * 100);      

    for(i = LEVEL_LEAST_CHILDREN; i < maxnv; i++)
        if((i % LEAST_CHILDREN_LEVEL_MOD == 0))
            fprintf(stderr, "Nv=%d, times no children even crown: %llu (%.2f%%)\n", i, times_no_children_even_crown[i],
                (double) times_no_children_even_crown[i] / (times_no_children_even_crown[i] + times_children_even_crown[i]) * 100);      
*/

    for(i = 5; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, num non-iso graphs gen: %llu (incl. iso: %llu), noniso: %.2f%%\n", i,
                num_graphs_generated_noniso[i], num_graphs_generated_noniso[i] + num_graphs_generated_iso[i],
                (double) num_graphs_generated_noniso[i] / (num_graphs_generated_noniso[i] + num_graphs_generated_iso[i]) * 100);

/*
    for(i = 9; i <= maxnv; i++)
        fprintf(stderr, "Nv=%d, times vertex-critical: %llu (%.2f%%)\n", i, times_graph_vertex_critical[i],
                (double) times_graph_vertex_critical[i] / (times_graph_vertex_critical[i] + times_graph_not_vertex_critical[i]) * 100);        
*/

    if(output_vertex_critical_graphs)
        fprintf(stderr, "Number of vertex-critical graphs written: %llu\n", num_graphs_written);
    else {
#ifdef TEST_IF_CRITICAL
        fprintf(stderr, "Number of critical graphs written: %llu\n", num_graphs_written);
#else
        fprintf(stderr, "Number of potential critical graphs written: %llu\n", num_graphs_written);
#endif
    }
    fprintf(stderr, "CPU time: %.1f seconds.\n", (double) (savetime / time_factor));

    return (EXIT_SUCCESS);
}