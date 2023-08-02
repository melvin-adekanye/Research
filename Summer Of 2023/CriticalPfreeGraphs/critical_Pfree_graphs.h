/**
 * critical_Pfree_graphs.h
 *
 * THIS CODE IS HELL OTHERWISE SO I MADE A HEADER FILE - Thaler Knodel
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
//#include "nauty.h"
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



/**** methods ****/
/**
 * Prints the given nauty graph in sparse representation.
 */
static void print_sparse_graph_nauty(sparsegraph sparse_graph);

/**
 * Prints the given nauty graph.
 */
static void printgraph_nauty(graph *g, int current_number_of_vertices);

static void printgraph_minibaum(GRAPH g, ADJACENCY adj, int nv);

static void printgraph(GRAPH g, ADJACENCY adj, int nv);

/*********************Methods for splay tree*********************************/

void new_splaynode(SPLAYNODE *el, graph *canong, int *is_new_node);

void old_splaynode(SPLAYNODE *el, int *is_new_node);

int comparenodes(graph *canong, SPLAYNODE *list);

// added int here so that it is not so ambiguous
int outputnode(SPLAYNODE *liste);

/* not splaytree stuff */

/* Assumes that MAXM is 1 (i.e. one_word_sets) */
//This is slightly faster...
int nextelement1(set *set1, int m, int pos);

/*
 * Converts g to a graph in the nauty format and stores the resulting graph
 * in nauty_graph.
 */
static void copy_nauty_graph(GRAPH g, ADJACENCY adj, int num_vertices, graph *nauty_graph);

/**
 * Code the graph using the multicode format and returns the length of the code.
 */
int code_multicode(unsigned char code[], GRAPH g, ADJACENCY adj, int num_of_vertices);

static void add_edge_zz(GRAPH current_graph, unsigned char adj[], unsigned char v, unsigned char w);

static void decode_multicode_zz(unsigned char code[], GRAPH current_graph, ADJACENCY adj, int codelength);

//Outputs the graph to stdout in multicode
static void output_graph(GRAPH g, ADJACENCY adj, int num_vertices);

/**
 * Searches for pairs of (nonadjacent) vertices in current_graph_mb which cannot have the same colour.
 * For these pairs an edge is added in nauty_graph_col_dom.
 *
 * This is tested by adding a diamond-gadget.
 */
//Important: it is assumed that current_graph_mb is filled in and represents the graph
//where certain vertices are removed (actually they are not removed but are isolated vertices)
//Important: vertices in removed_vertices are labelled starting from 0, not 1!
static void search_for_hidden_edges(setword removed_vertices, graph *nauty_graph_col_dom);

/**
 * Transforms current_graph into current_graph_mb where the vertices from
 * removed_vertices are isolated.
 */
//Important: vertices in removed_vertices are labelled starting from 0, not 1!
static void make_graph_with_removed_vertices(setword removed_vertices);


/***** Colour domination functions *****/


//Compute the colour domination graph of a given removed vertex
static void compute_colour_domination_graphs_vertex_removed_given(unsigned char removed_vertex);

/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - v.
 */
//Important: it is assumed that the col dom graph of vertices which are ISMARKED2
//were already determined!
static void compute_colour_domination_graphs_vertex_removed();

/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - e.
 */
static void compute_colour_domination_graphs_edge_removed();


/**
 * Computes all colour domination graphs of current_graph G (which is assumed to be k-colourable).
 * The colour domination graphs are supergraphs of G with the same vertices, but contain an edge
 * which is not in G between 2 vertices which cannot be k-coloured with the same
 * colour in G - triangle.
 */
static void compute_colour_domination_graphs_triangle_removed();




/**
 * Adds a new vertex and connects it will all vertices of
 * the vertex set.
 */
static void expand_vertex_set_no_remove(unsigned char dominating_set[], int set_size);

/**
 * Reduction of expand_dominating_set_no_remove()
 */
static void reduce_vertex_set_no_remove(unsigned char dominating_set[], int set_size);


/**
 * Returns 1 if current_set contains at least one element of forced_vertices_at_least_one,
 * else returns 0.
 * Also returns 0 if forced_vertices_at_least_one is empty
 */
static int contains_forced_vertices_at_least_one(setword current_set_bitvector,
                                                 setword forced_vertices_at_least_one);

//Returns 1 if the graph contains a triangle which also contains the last vertex, else returns 0
//The last vertex must be in the triangle, since the parents were triangle-free
//Important: it is assumed that the vertices of the last vertex are current_set_bitvector
static int contains_triangle_without_expand(unsigned char current_set[],
                                            int current_set_size,
                                            setword current_set_bitvector);

//Returns 1 if the graph contains a 4-cycle which also contains the last vertex, else returns 0
//Important: the 4-cycle is not necessarily induced!
static int countains_fourcycle_without_expand(unsigned char current_set[], int current_set_size);

//In case of similar vertices: num possible expansions = 2^(n-2)
static void construct_and_expand_vertex_sets_forbidden_vertices(unsigned char current_set[],
                                                                int current_set_size,
                                                                setword current_set_bitvector,
                                                                int next_vertex,
                                                                setword forbidden_vertices,
                                                                setword isolated_vertices,
                                                                unsigned char avoided_vertex,
                                                                int max_colours_nbh_colouring,
                                                                unsigned char destroyed_small_vertex,
                                                                setword forced_vertices_at_least_one,
                                                                int perform_extensions);


void colournext(GRAPH graph, int *colours, int tiefe, int *minsofar, int usedsofar,
                unsigned int forbidden[], int *nofc, unsigned int *MASK, int genug, int *good_enough);


/* returns the chromatic number from the first found colour set with a maximum number
   of colours given by "genug". Returns 0 if there is no colouring combination found.

   If genug = 0, the minimum colour combination is calculated
*/
/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
int chromatic_number(GRAPH graph, ADJACENCY adj, int genug);


/* returns the number of colours found in the first valid colour combination with the
   maximum number of colours given by "genug". Returns 0 if no combination is found.

   If genug = 0, the minimum colour combination is calculated
*/
/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
//In this modified method all vertices in same_colour_vertices get colour 1.
//Important: vertices are labelled from 0 in same_colour_vertices (not from 1)!
int chromatic_number_same_colour_vertices(GRAPH graph, ADJACENCY adj, int genug,
                                          setword same_colour_vertices);

/* returns the number of colours found in the first valid colour combination with the
   maximum number of colours given by "genug". Returns 0 if no combination is found.

   If genug = 0, the minimum colour combination is calculated
*/
/* gibt die Anzahl der Farben bei der zuerst gefundene Faerbungsmoeglichkeit mit
   maximal "genug" Farben zurueck. 0 falls keine gefunden wird.

   Im Falle genug==0 wird die beste Faerbung berechnet.
 */
//In this modified method all vertices in vertices[] get colour given_colours[i]
//Important: vertices are labelled from 0 in given_colours[] and colours start from 1 in given_colours[]
//Also important: it is assumed that the startcolouring is a valid colouring
int chromatic_number_given_startcolouring(GRAPH graph, ADJACENCY adj, int genug,
                                          unsigned char vertices[], int vertices_size, unsigned char given_colours[]);


//Returns 1 if current_graph is k-colourable, else returns 0
static int is_colourable();

//It is assumed that current_pathlength > 0
//Determines induced paths which do not contain any vertices from current_path_with_forbidden_neighbourhood
static void determine_longest_disjoint_induced_path_recursion(int maxlength);

// Stores as a queue
static void store_current_path();

/**
 * Returns 1 if the graph contains an induced path with a given size where none of
 * the vertices of the path is in forbidden_vertices, else returns 0.
 */
static int contains_disjoint_forbidden_path(setword forbidden_vertices);

//It is assumed that current_pathlength > 0
static void determine_longest_induced_path_recursion(int maxlength);

/**
 * Returns 1 if the vertices in possible_induced_path[] form an induced path of
 * with maxlength + 1 vertices, else returns 0.
 */
static int previous_induced_path_is_still_induced(int possible_induced_path[], int maxlength);


/**** Subgraph checking functions ****/


//Returns 1 if current_graph contains a forbidden induced path, else returns 0
static int contains_forbidden_induced_path();

//It is assumed that current_pathlength > 0
static void contains_forbidden_induced_cycle_recursion(int forbiddencyclesize);

/**
 * Returns 1 if the graph contains an induced cycle of size forbiddencyclesize,
 * else returns 0.
 */
//Since the parents were C-free, the last vertex must be in the forbidden cycle
static int contains_forbidden_induced_cycle(int forbiddencyclesize);

//Warning: 4-cycle is not necessarily induced!!!
static int countains_fourcycle();

/**
 * Returns 1 if the graph contains an induced cycle of size forbiddencyclesize,
 * else returns 0.
 */
//Testing all vertices, not just the last one!
static int contains_forbidden_induced_cycle_all(int forbiddencyclesize);

//Returns 1 if the graph contains a W5 which also contains the last vertex, else returns 0
//The last vertex must be in the W5, since the parents were W5-free
//int n: nv of the graph
//Important: it is assumed that the graph is K4-free, otherwise this might give wrong results
static int contains_W5();

//Returns 1 if the graph contains a K5 which also contains the last vertex, else returns 0
//The last vertex must be in the K5, since the parents were K5-free
//int n: nv of the graph
static int contains_K5();

//Returns 1 if the graph contains a K4 which also contains the last vertex, else returns 0
//The last vertex must be in the K4, since the parents were K4-free
//int n: nv of the graph
static int contains_K4();

//Returns 1 if the graph contains a diamond which also contains the last vertex, else returns 0
//The last vertex must be in the diamond, since the parents were diamond-free
//int n: nv of the graph
//Important: this method just tests if the graph contains a diamond as a subgraph,
//not necessarily as an induced subgraph (so it could contain a diamond in a K4)!
static int contains_diamond();

//Returns 1 if the graph contains a diamond which also contains the last vertex, else returns 0
//The last vertex must be in the diamond, since the parents were diamond-free
//int n: nv of the graph
static int contains_diamond_induced();

//Returns 1 if the graph contains a triangle which also contains the last vertex, else returns 0
//The last vertex must be in the triangle, since the parents were triangle-free
//int n: nv of the graph
static int contains_triangle();



static void sort_array_bubblesort(unsigned char vertexset[], int size);

//compare_array contains the tuple to which array should be compared
//If is_sim_pair = 1, best_pair[] contains:
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
//Where N(ai) \seteq N(bi)
static void make_all_possible_combinations(unsigned char array[], int array_size,
                               unsigned char current_array[], int current_array_size, unsigned char compare_array[],
                               unsigned char best_pair[], int use_col_dom_graph);

//Tests if the pair is a similar pair.
//Important: the total size of possible_similar_pair is assumed to be 2*size_single
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
static int is_similar_pair(unsigned char possible_similar_pair[], int size_single, int use_col_dom_graph);

//Tests if the pair is a similar pair.
//Important: the total size of possible_similar_pair is assumed to be 2*size_single
//Fixes the first x elements
static int is_similar_pair_fix_first_elements(unsigned char possible_similar_pair[], int size_single,
                                   int use_col_dom_graph, int num_first_elements_to_fix);

static int cycle_was_already_stored(setword cycle_bitvector);

/**
 * Returns 1 if the cubic cycle is a valid crown, else returns 0.
 *
 * All even cycles are valid, but only odd crowns where there is no colouring
 * such that the neighbours of the odd cycle all have the same colour are allowed!
 */
static int is_valid_crown(setword cycle_bitvector, int cycle_size);

//It is assumed that current_pathlength > 0
static void search_forbidden_induced_cycles_recursion(int forbiddencyclesize);

static void search_forbidden_induced_cycles(setword possible_vertices, int forbiddencyclesize);

/**
 * Returns 1 if the graph contains an even crown (i.e. an even cubic cycle).
 */
//TODO: probably even doesn't have to be an induced cycle!
static int contains_even_crown(setword *crown_vertices);

/**
 * Returns 1 if the graph contains an odd crown (i.e. an even cubic cycle).
 *
 * Important: not any odd crown is allowed! Only odd crowns where there is no colouring
 * such that the neighbours of the odd cycle all have the same colour are allowed!
 */
//TODO: probably even doesn't have to be an induced cycle!
static int contains_odd_crown(setword *crown_vertices);

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
static int contains_similar_diamond(unsigned char best_similar_diamondpair[8], int use_col_dom_graph);

static int contains_similar_P6(unsigned char best_similar_P6pair[12], int use_col_dom_graph);

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
static int contains_similar_P4(unsigned char best_similar_P4pair[8], int use_col_dom_graph);

//Returns 1 if the graph contains a disjoint similar edgepair (see Sawada et al. Lemma 3.1 case m=2)
//else returns 0.
//If 1 is returned, best_similar_edgepair points to the *best* similar vertex which should be destroyed in the next step
//best_similar_trianglepair[0] = a1
//best_similar_trianglepair[1] = a2
//best_similar_trianglepair[2] = a3
//best_similar_trianglepair[3] = b1
//best_similar_trianglepair[4] = b2
//best_similar_trianglepair[5] = b3
static int contains_similar_triangle(unsigned char best_similar_trianglepair[6], int use_col_dom_graph);

//Returns 1 if the graph contains a disjoint similar edgepair (see Sawada et al. Lemma 3.1 case m=2)
//else returns 0.
//If 1 is returned, best_similar_edgepair points to the *best* similar vertex which should be destroyed in the next step
//best_similar_edgepair[0] = a1
//best_similar_edgepair[1] = a2
//best_similar_edgepair[2] = b1
//best_similar_edgepair[3] = b2
static int contains_similar_edgepair(unsigned char best_similar_edgepair[4], int use_col_dom_graph);

//Returns 1 if the graph contains similar vertices (i.e. if there are 2 vertices u,v for which N(u) \seteq N(v))
//else returns 0.
//If 1 is returned, best_small_similar_vertex points to the *best* similar vertex which should be destroyed in the next step
//and other_larger_similar_vertex is the vertex which has a larger (or same) neighbourhood than best_small_similar_vertex
//The best similar vertex is the one with the largest degree (is significantly faster than taking a random one)
static int contains_similar_vertices(unsigned char *best_small_similar_vertex, unsigned char *other_larger_similar_vertex);

//Returns 1 if the graph contains similar vertices (i.e. if there are 2 vertices u,v for which N(u) \seteq N(v))
//else returns 0.
//If 1 is returned, best_small_similar_vertex points to the *best* similar vertex which should be destroyed in the next step
//and other_larger_similar_vertex is the vertex which has a larger (or same) neighbourhood than best_small_similar_vertex
//The best similar vertex is the one with the largest degree (is significantly faster than taking a random one)
static int contains_similar_vertices_col_dom(unsigned char *best_small_similar_vertex, unsigned char *other_larger_similar_vertex);

//(k+1)-critical graphs must have min degree >= k
//Returns 1 if the graphs contains a vertex with degree < k (= num_colours), else returns 0
//If 1 is returned, best_small_vertex points to the *best* small vertex
//It doesn't matter if the first one or the smallest or largest one is taken
//Girth case: seems best always to take small vertex with largest label!
static int contains_too_small_vertex(unsigned char *best_small_vertex);

//Returns 1 if the graph is vertex critical (i.e. all of its children is k-colourable)
//else returns 0
//Important 1: it is assumed that the graph is NOT k-colourable
//Important 2: we don't need to remove the last vertex since we know the parent is k-colourable
static int is_vertex_critical();




/**** Edge/vertex manipulation functions ****/

static void remove_edge_from_neighbourhood(unsigned char vertex, unsigned char removed_vertex);

static void add_vertex_to_neighbourhood(unsigned char vertex, unsigned char new_neighbour);

static void remove_edges_in_all_possible_ways(int edge_index, int num_edges_removed);


/**
 * Returns 1 if the graph is edge-critical, else returns 0.
 *
 * Important: this method is expensive, so it is important for the efficiency to
 * test vertex-criticality first.
 */
static int is_edge_critical();

//Returns 1 if the new vertex n can be coloured with the same colour as avoided_vertex,
//else returns 0
//If it cannot be coloured with the same colour (i.e. 0 is returned), the similar vertex
//wasn't destroyed by the expansion
//If isolated_vertices == 0, 1 is also returned
static int similar_element_is_destroyed(setword isolated_vertices, unsigned char avoided_vertex);

/**
 * Returns 1 if all previously destroyed vertices are still destroyed, else returns 0.
 */
//Important: the removed vertices for which the colour dom graphs are computed are
//marked with MARKS2
static int previously_destroyed_similar_vertices_are_still_destroyed();

/**
 * Adds the similar vertices to the list of destroyed_similar_vertices.
 */
static void add_similar_vertices_to_list(unsigned char small_similar_vertex, unsigned char larger_similar_vertex);

static void make_all_possible_sets_forced_colours(unsigned char possible_vertices[], int possible_vertices_size, int current_index,
                                                  unsigned char selected_vertices[], int selected_vertices_size,
                                                  int desired_selected_vertices_size, unsigned char given_colours[]);

static void colour_vertices_in_all_possible_ways(unsigned char vertices[], int vertices_size, int current_index,
                                     unsigned char colours[], int num_desired_colours, setword forbidden_colour_vertices[],
                                     setword colours_already_used);

//Determines the max num of colours used in any valid colouring of N(v) in G-v
//At most given_max_colours are used
//Note that G-v could be uncolourable (in that case 1 is returned)
static int max_colours_in_valid_neighbourhood_colouring(unsigned char vertex, int given_max_colours);

//Test if there is a valid colouring of G-v where vertices of N(v) are coloured with k colours
//for every v in V(G)
//Returns 0 if it is the case, else returns 1
//If 1 is returned, best_small_vertex points to the *best* small vertex
//If 1 is returned, this means that G cannot be critical and that best_small_vertex must be destroyed
static int scontains_small_colour_dominated_vertex(unsigned char *best_small_vertex);

/**
 * Returns the number of edges of the current graph.
 */
static int determine_number_of_edges();

/**
 * Returns 1 if the current graph is planar, else returns 0.
 *
 * Uses planarity.c - code for planarity testing of undirected graphs by
 * Boyer and Myrvold.
 */
static int is_planar();

/**
 * Returns 1 if there is an expansion without any children, else returns 0.
 *
 * Important: in case 1 is returned and in case the expansion has uncolourable children,
 * it is assumed that these uncolourable children were already output!
 */
static int contains_expansion_without_any_children();

/**
 * DFS algorithm which recursively marks all vertices which can be reached from
 * current_vertex.
 * Complexity: O(|V|+|E|)
 */
static void mark_dfs(unsigned char current_vertex);

/**
 * Returns the number of connectivity components in the graph when the vertices
 * in the bitvector forbidden_vertices are removed.
 */
static int number_of_components_when_vertices_removed(setword forbidden_vertices);

/**
 * Returns 1 if the graph contains a cutvertex, else returns 0.
 * If 1 is returned, best_cutvertex points to the *best* cutvertex and
 * number_of_components contains the number of connectivity components when
 * best_cutvertex is removed.
 */
static int contains_cutvertex(unsigned char *best_cutvertex, int *number_of_components, setword *best_comp);

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
                                         int next_index, unsigned char current_set[], int current_set_size,
                                         setword forbidden_vertices, unsigned char central_vertex);

static int triangle_combination_leads_to_bull(int a0, int a1, int a2);

/**
 * Returns 1 if the graph contains an induced bull, else returns 0.
 */
//TODO: can remember previous induced bulls, but this method is no bottleneck?
static int contains_induced_bull();

/**
 * Returns 1 if the graph contains an induced fork, else returns 0.
 * A fork is obtained from K_{1,4} by subdividing two edges (so it has 7 vertices).
 */
//TODO: can remember previous induced forks, but this method is no bottleneck?
static int contains_induced_fork();

static int test_if_combination_yields_gem(TRIANGLE triangle, int central_vertex, int central_vertex_index);

/**
 * Returns 1 if the graph contains an induced gem, else returns 0.
 */
//Graph6 code gem: Dh{
//Since the parent graph was gem-free, the last vertex must be in the gem
//But not doing this since this is a bit tricky as a gem has 3 vertex-orbits
static int contains_induced_gem();

// M+C June 26, 2023
// Function to check if a graph is P4+P1-free
static int contains_induced_p4_p1();

/*
 * Returns 1 if the graph contains an induced copy, else returns 0.
 */
 
static int contains_induced_copy();

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
static void extend(setword isolated_vertices, unsigned char avoided_vertex,
                   int max_colours_nbh_colouring, unsigned char destroyed_small_vertex, int perform_extensions);

static void initialize_splitting();

//Makes a cycle of a given size
static void make_complete_graph(int size);

//Makes a cycle of a given size
static void make_cycle(int cycle_size);

//Makes an antihole of a given size
static void make_antihole(int size);

static void add_edge(GRAPH current_graph, unsigned char adj[], unsigned char v, unsigned char w);

/**
 * Decodes the code (which is in multicode format) of a graph.
 */
static void decode_multicode(unsigned char code[], GRAPH current_graph, ADJACENCY adj, int codelength);

static void init_construction();

static void perform_wordsize_checks();

// prints help
static void print_help(char * argv0);