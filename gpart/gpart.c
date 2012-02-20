/*
 * gpart.c: Graph partitioning library.
 *
 * This library includes multilevel algorithms for partitioning weighted graphs.
 * Algorithm is based on papers [1-5].
 *
 * [1] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 * [2] B. Hendrickson and R.W. Leland. A Multi-Level Algorithm For Partitioning
 *     Graphs // Proceedings of ACM/IEEE conference on Supercomputing, 1995.
 * [3] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 * [4] B.W. Kernighan and S. Lin. An Efficient Heuristic Procedure for
 *     Partitioning Graphs // Bell System Technical Journal, Vol. 49,
 *     1970. - P. 291-307.
 * [5] C.M. Fiduccia and R.M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#include "gpart.h"
#include "refine_fm.h"
#include "coarsen.h"

#define COARSE_GRAPH_SIZE_MAX     21
#define MULTIVERTEX_WEIGHT_SCALE  1.45
#define BISECTION_LND_ITERS_MAX   4

static int gpart_partition_recursive_lnd(csrgraph_t *g, gpart_options_t *opt,
		                                 int *pweights, int nparts, int *part,
		                                 int startpartno);

static int gpart_partition_recursive_lnd_regular(csrgraph_t *g,
		                                         gpart_options_t *opt,
		                                         int *pweights, int nparts,
		                                         int *part, int startpartno);

static int gpart_bisect_multilevel(csrgraph_t *g, gpart_options_t *opt,
		                           int *partsizes, int *bisection);

static int gpart_bisect_multilevel_regular(csrgraph_t *g, gpart_options_t *opt,
		                                   int *partsizes, int *bisection);

static int gpart_bisect_lnd(csrgraph_t *g, gpart_options_t *opt, int *partsizes,
		                    int *bisection);

static int gpart_bisect_lnd_regular(csrgraph_t *g, gpart_options_t *opt,
		                            int *partsizes, int *bisection);

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
int gpart_partition_recursive(csrgraph_t *g, int *partweights, int nparts,
		                      int *part)
{
	int i, w;
	gpart_options_t opt;

	if (nparts > g->nvertices)
		return 0;
	for (w = 0, i = 0; i < nparts; i++) {
		if (partweights[i] == 0 || partweights[i] == g->nvertices)
			return 0;
		w += partweights[i];
	}
	if (w != g->nvertices)
		return 0;

	opt.coarse_graph_size = COARSE_GRAPH_SIZE_MAX;
	opt.vweight_max = 1;
	if (g->nvertices > opt.coarse_graph_size) {
		opt.vweight_max = (int)(MULTIVERTEX_WEIGHT_SCALE
		                        * (g->nvertices / opt.coarse_graph_size));
	}

	srand(0);
	return gpart_partition_recursive_lnd(g, &opt, partweights, nparts, part, 0);
}

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
		                              int nparts, int *part)
{
	int i, w;
	gpart_options_t opt;

	if (nparts > g->nvertices)
		return 0;
	for (w = 0, i = 0; i < nparts; i++) {
		if (partweights[i] == 0 || partweights[i] == g->nvertices)
			return 0;
		w += partweights[i];
	}
	if (w != g->nvertices)
		return 0;

	opt.coarse_graph_size = COARSE_GRAPH_SIZE_MAX;
	opt.vweight_max = 1;
	if (g->nvertices > opt.coarse_graph_size) {
		opt.vweight_max = (int)(MULTIVERTEX_WEIGHT_SCALE
		                        * (g->nvertices / opt.coarse_graph_size));
	}

	srand(0);
	return gpart_partition_recursive_lnd_regular(g, &opt, partweights, nparts,
			                                     part, 0);
}

/*
 * gpart_partition_recursive_lnd:
 *
 * Partition graph g into nparts subsets by recursive bisection.
 * Return 0 on success and 1 otherwise.
 */
int gpart_partition_recursive_lnd(csrgraph_t *g, gpart_options_t *opt,
		                          int *partweights, int nparts, int *part,
		                          int startpartno)
{
	int i, rc = 0;
	int nvertices;
	csrgraph_t *graphs[2] = {NULL, NULL};
	int *bisection = NULL, *vparents;
	int pw[2], pwresult[2], nmoves, moveto;

	nvertices = g->nvertices;
	if ( (bisection = malloc(sizeof(*bisection) * nvertices)) == NULL)
		return 1;

	/* Setup limits on partition sizes */
	for (pw[0] = 0, i = 0; i < nparts / 2; i++)
		pw[0] += partweights[i];		   /* Weight of 50% of vertices */
	pw[1] = nvertices - pw[0];		   /* 100% - 50% of vertices */

	/* Bisect graph */
	rc += gpart_bisect_multilevel(g, opt, pw, bisection);

	/* Balance size of partitions */
	pwresult[0] = 0;
	pwresult[1] = 0;
	for (i = 0; i < nvertices; i++)
		pwresult[bisection[i]]++;
	nmoves = abs(pwresult[0] - pw[0]);
	moveto = (pwresult[0] > pw[0]) ? 1 : 0;
	if (nmoves > 0)
		gpart_balance_bisection_fm(g, bisection, nmoves, moveto);

	/* Save bisection */
	if ((vparents = g->vparents) != NULL) {
		for (i = 0; i < nvertices; i++)
			part[vparents[i]] = bisection[i] + startpartno;
	} else {
		for (i = 0; i < nvertices; i++)
			part[i] = bisection[i] + startpartno;
	}

	/* Divide graph on two subgraphs */
	if (nparts > 2) {
		csrgraph_bisect(g, bisection, &graphs[0], &graphs[1]);
	}
	free(bisection);

	/* Recursive partition subgraphs */
	if (nparts > 3) {
		rc += gpart_partition_recursive_lnd(graphs[0], opt, partweights,
				                            nparts / 2, part, startpartno);
		rc += gpart_partition_recursive_lnd(graphs[1], opt,
				                            partweights + nparts / 2,
				                            nparts - nparts / 2, part,
				                            startpartno + nparts / 2);
	} else if (nparts == 3) {
		rc += gpart_partition_recursive_lnd(graphs[1], opt,
				                            partweights + nparts / 2,
				                            nparts - nparts / 2, part,
				                            startpartno + nparts / 2);
	}

	csrgraph_free(graphs[0]);
	csrgraph_free(graphs[1]);

	return rc > 0 ? 1 : 0;
}

/*
 * gpart_partition_recursive_lnd_regular:
 *
 * Partition regular graph g into nparts subsets by recursive bisection.
 * Return 0 on success and 1 otherwise.
 */
int gpart_partition_recursive_lnd_regular(csrgraph_t *g, gpart_options_t *opt,
		                                  int *partweights, int nparts,
		                                  int *part, int startpartno)
{
	int i, rc = 0;
	int nvertices;
	csrgraph_t *graphs[2] = {NULL, NULL};
	int *bisection = NULL, *vparents;
	int pw[2], pwresult[2], nmoves, moveto;

	nvertices = g->nvertices;
	if ( (bisection = malloc(sizeof(*bisection) * nvertices)) == NULL)
		return 1;

	/* Setup limits on partition sizes */
	for (pw[0] = 0, i = 0; i < nparts / 2; i++)
		pw[0] += partweights[i];		   /* Weight of 50% of vertices */
	pw[1] = nvertices - pw[0];		   /* 100% - 50% of vertices */

	/* Bisect graph */
	rc += gpart_bisect_multilevel_regular(g, opt, pw, bisection);

	/* Balance size of partitions */
	pwresult[0] = 0;
	pwresult[1] = 0;
	for (i = 0; i < nvertices; i++)
		pwresult[bisection[i]]++;
	nmoves = abs(pwresult[0] - pw[0]);
	moveto = (pwresult[0] > pw[0]) ? 1 : 0;
	if (nmoves > 0)
		gpart_balance_bisection_fm(g, bisection, nmoves, moveto);

	/* Save bisection */
	if ((vparents = g->vparents) != NULL) {
		for (i = 0; i < nvertices; i++)
			part[vparents[i]] = bisection[i] + startpartno;
	} else {
		for (i = 0; i < nvertices; i++)
			part[i] = bisection[i] + startpartno;
	}

	/* Divide graph on two subgraphs */
	if (nparts > 2) {
		csrgraph_bisect(g, bisection, &graphs[0], &graphs[1]);
	}
	free(bisection);

	/* Recursive partition subgraphs */
	if (nparts > 3) {
		rc += gpart_partition_recursive_lnd_regular(graphs[0], opt, partweights,
				                                    nparts / 2, part,
				                                    startpartno);
		rc += gpart_partition_recursive_lnd_regular(graphs[1], opt,
				                                    partweights + nparts / 2,
				                                    nparts - nparts / 2, part,
				                                    startpartno + nparts / 2);
	} else if (nparts == 3) {
		rc += gpart_partition_recursive_lnd_regular(graphs[1], opt,
				                                    partweights + nparts / 2,
				                                    nparts - nparts / 2, part,
				                                    startpartno + nparts / 2);
	}

	csrgraph_free(graphs[0]);
	csrgraph_free(graphs[1]);

	return rc > 0 ? 1 : 0;
}

/*
 * gpart_bisect_multilevel: Bisect graph using multilevel approach [*].
 *                          Return 0 on success and 1 otherwise.
 *
 * [*] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 */
int gpart_bisect_multilevel(csrgraph_t *g, gpart_options_t *opt, int *partsizes,
		                    int *bisection)
{
	coarse_graph_t *cg = NULL;

	/* Build list of coarse graphs: |V0| < |V1| < ... < |Vm| */
	cg = gpart_coarsen_graph(g, opt);

	/* Bisect coarse (small) graph by LND algorithm */
	if (cg != NULL)
		gpart_bisect_lnd(cg->graph, opt, partsizes, bisection);
	else
		gpart_bisect_lnd(g, opt, partsizes, bisection);

	/* Project partition bisection back and refine */
	if (cg != NULL)
		gpart_project_bisection(cg, g, bisection, opt, partsizes);

	return 0;
}

/*
 * gpart_bisect_multilevel_regular: Bisect regular graph using multilevel
 *                                  approach [*].
 *                                  Return 0 on success and 1 otherwise.
 *
 * [*] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 */
int gpart_bisect_multilevel_regular(csrgraph_t *g, gpart_options_t *opt,
		                            int *partsizes, int *bisection)
{
	coarse_graph_t *cg = NULL;

	/* Build list of coarse graphs: |V0| < |V1| < ... < |Vm| */
	cg = gpart_coarsen_graph_regular(g, opt);

	/* Bisect coarse (small) graph by LND algorithm */
	if (cg != NULL)
		gpart_bisect_lnd_regular(cg->graph, opt, partsizes, bisection);
	else
		gpart_bisect_lnd_regular(g, opt, partsizes, bisection);

	/* Project partition bisection back and refine */
	if (cg != NULL)
		gpart_project_bisection_regular(cg, g, bisection, opt, partsizes);

	return 0;
}

/* 
 * gpart_bisect_lnd:
 *
 * Bisect graph g by Levelized Nested Disection algorithm [1] and refine
 * bisection by Fiduccia-Mattheyses [2] heuristics.
 *
 * Return 0 on success and 1 otherwise.
 *
 * [1] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 * [2] C.M. Fiduccia and R. M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 */
int gpart_bisect_lnd(csrgraph_t *g, gpart_options_t *opt, int *partsizes,
		             int *bisection)
{
	int i, j, k, qtop, qtail, nleft, niters;
	int *queue, *visited;
	edgecut_t edgecut, edgecut_min = EDGECUT_MAX;
	int *bisection_best;
	int pw[2], pw_lim[2], sum;
	int small;

	queue = malloc(sizeof(*queue) * g->nvertices);
	visited = malloc(sizeof(*visited) * g->nvertices);
	bisection_best = malloc(sizeof(*bisection_best) * g->nvertices);

	if (queue == NULL || visited == NULL || bisection_best == NULL)
		return 1;

	for (niters = 0; niters < BISECTION_LND_ITERS_MAX; niters++) {

		/* Setup limits on partition sizes */
		for (i = 0, sum = 0; i < g->nvertices; i++)
			sum += g->vweights[i];
		pw_lim[0] = partsizes[0];
		pw_lim[1] = partsizes[1];
		pw[1] = partsizes[0] + partsizes[1];
		pw[0] = 0;
		small = 0;

		/* Construct bisection by breadth-first search */
		for (i = 0; i < g->nvertices; i++) {
			bisection[i] = 1;
			visited[i] = 0;
		}

		/* Start search from random vertex */
		queue[0] = (int)(g->nvertices * (double)rand() /
				         (double)(RAND_MAX + 1.0));
		visited[queue[0]] = 1;
		nleft = g->nvertices - 1;
		qtop = 0; qtail = 1;

		/* BFS */
		for (;;) {
			if (qtop == qtail) {
				if (nleft == 0 || small) {
					break;
				}

				/* Queue is empty. Search unvisited random vertex number k. */
				k = (int)(nleft * (double)rand() / (double)(RAND_MAX + 1.0));
				for (i = 0; i < g->nvertices; i++) {
					if (visited[i] == 0) {
						if (k == 0) {
							break;
						} else {
							k--;
						}
					}
				}
				queue[0] = i;
				visited[i] = 1;
				qtop = 0;
				qtail = 1;
				nleft--;
			}

			/* Visit vertex from queue */
			i = queue[qtop++];
			if (pw[0] > 0 && pw[1] - g->vweights[i] < pw_lim[1]) {
				small = 1;
				continue;
			}

			bisection[i] = 0;
			pw[0] += g->vweights[i];
			pw[1] -= g->vweights[i];

			if (pw[1] <= pw_lim[1]) {
				break;						/* Partition is done */
			}

			small = 0;
			for (j = g->adjindexes[i]; j < g->adjindexes[i + 1]; j++) {
				k = g->adjv[j];
				if (!visited[k]) {
					queue[qtail++] = k;
					visited[k] = 1;
					nleft--;
				}
			}

		} /* BFS for */

		/* Refine bisection */
		gpart_refine_bisection_fm(g, bisection, pw, &edgecut);

		if (edgecut_min > edgecut) {
			edgecut_min = edgecut;
			for (i = 0; i < g->nvertices; i++)
				bisection_best[i] = bisection[i];
		}

	} /* iters for */

	for (i = 0; i < g->nvertices; i++)
		bisection[i] = bisection_best[i];

	free(queue);
	free(visited);
	free(bisection_best);

	return 0;
}

/*
 * gpart_bisect_lnd_regular:
 *
 * Bisect regular graph g by Levelized Nested Disection algorithm [1] and refine
 * bisection by Fiduccia-Mattheyses [2] heuristics.
 *
 * Return 0 on success and 1 otherwise.
 *
 * [1] K. Schloegel, G. Karypis and V. Kumar. Graph Partitioning for
 *     High Performance Scientific Simulations // Sourcebook of parallel
 *     computing, 2003. - P. 491-541.
 * [2] C.M. Fiduccia and R. M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 */
int gpart_bisect_lnd_regular(csrgraph_t *g, gpart_options_t *opt,
		                     int *partsizes, int *bisection)
{
	int i, j, k, qtop, qtail, nleft, niters;
	int *queue, *visited;
	edgecut_t edgecut, edgecut_min = EDGECUT_MAX;
	int *bisection_best;
	int pw[2], pw_lim[2], sum;
	int small;

	queue = malloc(sizeof(*queue) * g->nvertices);
	visited = malloc(sizeof(*visited) * g->nvertices);
	bisection_best = malloc(sizeof(*bisection_best) * g->nvertices);

	if (queue == NULL || visited == NULL || bisection_best == NULL)
		return 1;

	for (niters = 0; niters < BISECTION_LND_ITERS_MAX; niters++) {

		/* Setup limits on partition sizes */
		for (i = 0, sum = 0; i < g->nvertices; i++)
			sum += g->vweights[i];
		pw_lim[0] = partsizes[0];
		pw_lim[1] = partsizes[1];
		pw[1] = partsizes[0] + partsizes[1];
		pw[0] = 0;
		small = 0;

		/* Construct bisection by breadth-first search */
		for (i = 0; i < g->nvertices; i++) {
			bisection[i] = 1;
			visited[i] = 0;
		}

		/* Start search from random vertex */
		queue[0] = (int)(g->nvertices * (double)rand() /
				         (double)(RAND_MAX + 1.0));
		visited[queue[0]] = 1;
		nleft = g->nvertices - 1;
		qtop = 0; qtail = 1;

		/* BFS */
		for (;;) {
			if (qtop == qtail) {
				if (nleft == 0 || small) {
					break;
				}

				/* Queue is empty. Search unvisited random vertex number k. */
				k = nleft * (double)rand() / (double)(RAND_MAX + 1.0);
				for (i = 0; i < g->nvertices; i++) {
					if (visited[i] == 0) {
						if (k == 0) {
							break;
						} else {
							k--;
						}
					}
				}
				queue[0] = i;
				visited[i] = 1;
				qtop = 0;
				qtail = 1;
				nleft--;
			}

			/* Visit vertex from queue */
			i = queue[qtop++];
			if (pw[0] > 0 && pw[1] - g->vweights[i] < pw_lim[1]) {
				small = 1;
				continue;
			}

			bisection[i] = 0;
			pw[0] += g->vweights[i];
			pw[1] -= g->vweights[i];

			if (pw[1] <= pw_lim[1]) {
				break;						/* Partition is done */
			}

			small = 0;
			for (j = g->adjindexes[i]; j < g->adjindexes[i + 1]; j++) {
				k = g->adjv[j];
				if (!visited[k]) {
					queue[qtail++] = k;
					visited[k] = 1;
					nleft--;
				}
			}

		} /* BFS for */

		/* Refine bisection */
		gpart_refine_bisection_fm(g, bisection, pw, &edgecut);

		if (edgecut_min > edgecut) {
			edgecut_min = edgecut;
			for (i = 0; i < g->nvertices; i++)
				bisection_best[i] = bisection[i];
		}

	} /* iters for */

	for (i = 0; i < g->nvertices; i++)
		bisection[i] = bisection_best[i];

	free(queue);
	free(visited);
	free(bisection_best);

	return 0;
}

/* gpart_compute_edgecut: Return partition edge-cut. */
edgecut_t gpart_compute_edgecut(csrgraph_t *g, int *part)
{
	int i, j;
	edgecut_t edgecut = 0;

	for (i = 0; i < g->nvertices; i++) {
		for (j = g->adjindexes[i]; j < g->adjindexes[i + 1]; j++) {
			if (part[i] != part[g->adjv[j]])
				edgecut += (edgecut_t)g->edges[j];
		}
	}
	edgecut /= 2;
	return edgecut;
}
