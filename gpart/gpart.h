/*
 * gpart.h: Graph partitioning library.
 *
 * This library includes multilevel algorithms for partitioning weighted graphs.
 * Algorithm is based on papers [1-5].
 *
 * [1] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 * [2] B. Hendrickson and R. W. Leland. A Multi-Level Algorithm For Partitioning
 *     Graphs // Proceedings of ACM/IEEE conference on Supercomputing, 1995.
 * [3] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 * [4] B.W. Kernighan and S. Lin. An Efficient Heuristic Procedure for
 *     Partitioning Graphs // Bell System Technical Journal, Vol. 49,
 *     1970. - P. 291-307.
 * [5] C.M. Fiduccia and R. M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 *
 */

#ifndef GPART_H
#define GPART_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compressed Sparse Row graph format.
 *
 * Adjacency list of vertex i is stored in array adjv starting at index
 * adjindexes[i] and ending at adjindexes[i + 1] - 1.
 * Length of array adjindexes is nvertices + 1.
 * Length of array adjv is 2 * nedges.
 * Example: adjv[adjindexes[3]] - is a number of first adj. vertex for node 3.
 *
 */
typedef struct csrgraph csrgraph_t;
struct csrgraph {
	int nvertices;		/* Number of vertices in graph */
	int nedges;			/* Number of edges in graph: 2 * number of edges */
	int *adjindexes;
	int *adjv;			/* List of adj. vertices */
	int *edges;			/* Weights of edges */

	int *vparents;		/* Map bisection vertices to parent vertices */
	int *vweights;		/* Weights of vertices (coarsening) */
	int *vmap;			/* Map source vertices to coarse graph (coarsening) */
};

#ifdef __USE_ISOC99
#	define EDGECUT_MAX ULLONG_MAX
	typedef unsigned long long int edgecut_t;
#else
#	define EDGECUT_MAX UINT_MAX
	typedef unsigned int edgecut_t;
#endif

typedef struct gpart_options gpart_options_t;
struct gpart_options {
	int coarse_graph_size;
	int vweight_max;		/* Limit on maximal weight of multivertex */
};

/*
 * gpart_partition_recursive:
 *
 * Partition graph g into nparts subsets by recursive bisection.
 * Bisection is constructed by Levelized Nested Disection (LND) algorithm [*].
 * Objective is minimize an edge-cut.
 *
 * Input:
 *     g is a weighted (edges) undirected graph.
 *     partweights is a array of required partition sizes (number of vertices).
 *     nparts is a number of required partitions.
 *
 * Output:
 *     part[i] - is a number of partition for vertex i.
 *
 * Return 0 on success and 1 otherwise.
 *
 * [*] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 */
int gpart_partition_recursive(csrgraph_t *g, int *partweights,
		                      int nparts, int *part);

/*
 * gpart_partition_recursive_regular:
 *
 * Partition regular graph g into nparts subsets by recursive bisection.
 * Bisection is constructed by Levelized Nested Disection (LND) algorithm [*].
 * Objective is minimize an edge-cut.
 *
 * Input:
 *     g is a weighted (edges) undirected regular graph.
 *     partweights is a array of required partition sizes (number of vertices).
 *     nparts is a number of required partitions.
 *
 * Output:
 *     part[i] - is a number of partition for vertex i.
 *
 * Return 0 on success and 1 otherwise.
 *
 * [*] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 */
int gpart_partition_recursive_regular(csrgraph_t *g, int *partweights,
		                              int nparts, int *part);

/* gpart_compute_edgecut: Return partition edge-cut. */
edgecut_t gpart_compute_edgecut(csrgraph_t *g, int *part);

/* csrgraph_create: Allocate memory for graph. */
csrgraph_t *csrgraph_create(int nvertices, int nedges);

/* csrgraph_load: Read graph in CSR format from file. */
csrgraph_t *csrgraph_load(const char *filename);

/* csrgraph_save: Save graph in file. */
int csrgraph_save(csrgraph_t *g, const char *filename);

/* csrgraph_free: Destroy graph. */
void csrgraph_free(csrgraph_t *graph);

/* csrgraph_get_edge_weight: Return weight of edge (i, j) or 0. */
int csrgraph_get_edge_weight(csrgraph_t *g, int i, int j);

/* csrgraph_bisect: Split graph on two subgraphs. */
int csrgraph_bisect(csrgraph_t *graph, int *bisection,
		            csrgraph_t **leftgraph, csrgraph_t **rightgraph);

/* csrgraph_generate_bruck: Generates task graph of Bruck's algorithm. */
csrgraph_t *csrgraph_generate_bruck(int nvertices);

/*
 * csrgraph_generate_recdoubling: Generates task graph of recursive doubling
 *                                algorithm.
 */
csrgraph_t *csrgraph_generate_recdoubling(int nvertices);

void csrgraph_print_adjmatrix(csrgraph_t *graph);

#ifdef __cplusplus
}
#endif

#endif /* GPART_H */
