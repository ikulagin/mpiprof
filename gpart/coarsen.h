/*
 * coarsen.h: Algorithms for coarsen graphs [1-2].
 *
 * [1] B. Hendrickson and R. W. Leland. A Multi-Level Algorithm For Partitioning
 *     Graphs // Proceedings of ACM/IEEE conference on Supercomputing, 1995.
 * [2] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#ifndef COARSEN_H
#define COARSEN_H

#include "gpart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct coarse_graph coarse_graph_t;
struct coarse_graph {
	csrgraph_t *graph;			/* Graph */
	coarse_graph_t *prev;		/* Pointer to finer graph */
	coarse_graph_t *next;		/* Pointer to coarse graph */
};

/*
 * gpart_coarsen_graph:
 *
 * Create list of coarse graphs by Heavy Edge Matching (HEM) algorithm [*] and
 * return pointer to coarse graph (last in list).
 *
 * [*] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 */
coarse_graph_t *gpart_coarsen_graph(csrgraph_t *g, gpart_options_t *opt);

/*
 * gpart_coarsen_graph_regular:
 *
 * Create list of coarse graphs by Heavy Edge Matching (HEM) algorithm [*] and
 * return pointer to coarse graph (last in list). Source graph is regular.
 *
 * [*] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 */
coarse_graph_t *gpart_coarsen_graph_regular(csrgraph_t *g, gpart_options_t *opt);

/*
 * gpart_project_bisection: Project bisection to source graph and refine it.
 *                          Return 0 on success and 1 otherwise.
 */
int gpart_project_bisection(coarse_graph_t *cg, csrgraph_t *srcgraph,
		                    int *bisection, gpart_options_t *opt,
		                    int *partsizes);

/*
 * gpart_project_bisection_regular: Project bisection to source regular graph
 *                                  and refine it.
 *                                  Return 0 on success and 1 otherwise.
 */
int gpart_project_bisection_regular(coarse_graph_t *cg, csrgraph_t *srcgraph,
		                            int *bisection, gpart_options_t *opt,
		                            int *partsizes);

#ifdef __cplusplus
}
#endif

#endif /* COARSEN_H */

