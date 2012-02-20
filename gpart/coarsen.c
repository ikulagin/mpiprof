/*
 * coarsen.c: Algorithms for coarsen graphs [1-2].
 *
 * [1] B. Hendrickson and R. W. Leland. A Multi-Level Algorithm For Partitioning
 *     Graphs // Proceedings of ACM/IEEE conference on Supercomputing, 1995.
 * [2] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "coarsen.h"
#include "gpart.h"
#include "refine_fm.h"

#define COARSENING_RATIO 0.91		 /* Coarse graph size / source graph size */
#define COARSENING_HEM_VDEGREE_LIMIT 1.0

static coarse_graph_t *coarsen_hem(coarse_graph_t *g, gpart_options_t *opt);
static coarse_graph_t *coarsen_hem_regular(coarse_graph_t *g,
		                                   gpart_options_t *opt);

static coarse_graph_t *create_coarse_graph(coarse_graph_t *g,
		                                   int coarse_nvertices,
		                                   int *traverse, int *match);
static coarse_graph_t *create_coarse_graph_regular(coarse_graph_t *g,
		                                           int coarse_nvertices,
		                                           int *traverse, int *match);

static int counting_sort(int *src, int *values, int n, int valmax, int *dst);

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
coarse_graph_t *gpart_coarsen_graph(csrgraph_t *g, gpart_options_t *opt)
{
	int i;
	coarse_graph_t *srcgraph = NULL, *cg = NULL, *p = NULL;
	int coarsen_flag;
	int *vw;

	/* Create head of list */
	if ( (srcgraph = malloc(sizeof(*srcgraph))) == NULL)
		return NULL;
	srcgraph->graph = g;
	srcgraph->prev = NULL;
	srcgraph->next = NULL;
	srcgraph->graph->vweights = malloc(sizeof(*srcgraph->graph->vweights)
			                           * srcgraph->graph->nvertices);
	if (srcgraph->graph->vweights == NULL) {
		free(srcgraph->graph->vweights);
		free(srcgraph);
		return NULL;
	}

	vw = srcgraph->graph->vweights;
	for (i = 0; i < srcgraph->graph->nvertices; i++)
		vw[i] = 1;

	if (opt->vweight_max == 1)
		return srcgraph;

	/* Build list of coarse graphs */
	cg = srcgraph;
	do {
		/* Coarsen current graph */
		cg->next = coarsen_hem(cg, opt);
		if (cg->next == NULL) {
			/* Destroy graphs */
			for ( ; cg->prev != NULL; cg = p) {
				p = cg->prev;
				csrgraph_free(cg->graph);
				free(cg);
			}
			cg->next = NULL;
			return NULL;
		}
		cg = cg->next;

		coarsen_flag = (cg->graph->nvertices > opt->coarse_graph_size)
				       && ((cg->graph->nvertices / 2) < cg->graph->nedges)
				       && cg->graph->nvertices
				          < COARSENING_RATIO * cg->prev->graph->nvertices;

	} while (coarsen_flag);

	return cg;
}

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
coarse_graph_t *gpart_coarsen_graph_regular(csrgraph_t *g, gpart_options_t *opt)
{
	int i;
	coarse_graph_t *srcgraph = NULL, *cg = NULL, *p = NULL;
	int coarsen_flag;
	int *vw;

	/* Create head of list */
	if ( (srcgraph = malloc(sizeof(*srcgraph))) == NULL)
		return NULL;
	srcgraph->graph = g;
	srcgraph->prev = NULL;
	srcgraph->next = NULL;
	srcgraph->graph->vweights = malloc(sizeof(*srcgraph->graph->vweights)
			                           * srcgraph->graph->nvertices);
	if (srcgraph->graph->vweights == NULL) {
		free(srcgraph->graph->vweights);
		free(srcgraph);
		return NULL;
	}

	vw = srcgraph->graph->vweights;
	for (i = 0; i < srcgraph->graph->nvertices; i++)
		vw[i] = 1;

	if (opt->vweight_max == 1)
		return srcgraph;

	/* Build list of coarse graphs */
	cg = srcgraph;
	do {
		/* Coarsen current graph */
		cg->next = coarsen_hem_regular(cg, opt);

		if (cg->next == NULL) {
			/* Destroy graphs */
			for ( ; cg->prev != NULL; cg = p) {
				p = cg->prev;
				csrgraph_free(cg->graph);
				free(cg);
			}
			cg->next = NULL;
			return NULL;
		}
		cg = cg->next;

		coarsen_flag = (cg->graph->nvertices > opt->coarse_graph_size)
				       && ((cg->graph->nvertices / 2) < cg->graph->nedges)
				       && cg->graph->nvertices
				          < COARSENING_RATIO * cg->prev->graph->nvertices;

	} while (coarsen_flag);

	return cg;
}


/*
 * coarsen_hem: Coarsen graph by Heavy Edge Matching (HEM) heuristic.
 *              Return pointer to coarse graph.
 */
coarse_graph_t *coarsen_hem(coarse_graph_t *g, gpart_options_t *opt)
{
	csrgraph_t *graph;
	int nvertices, *adjindexes;
	int err = 0;
	int i, j, k, v;
	coarse_graph_t *cg = NULL;
	int *match = NULL;
	int *vperm = NULL;
	int *traverse = NULL;
	int vdegavg, *vdeg = NULL;
	int coarse_nvertices;
	int coarse_nedges;
	int vmax, wmax;

	graph = g->graph;
	nvertices = g->graph->nvertices;
	adjindexes = g->graph->adjindexes;

	if ( (graph->vmap = malloc(sizeof(*graph->vmap)
			                      * nvertices)) == NULL)
	{
		err = 1;
		goto errhandler;
	}

	if ( (match = malloc(sizeof(*match) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (vperm = malloc(sizeof(*vperm) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (traverse = malloc(sizeof(*traverse) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (vdeg = malloc(sizeof(*vdeg) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Permute vertices for random traverse of graph */
	for (i = 0; i < nvertices; i++) {
		vperm[i] = i;
	}

	for (i = 0; i < nvertices; i++) {
		j = nvertices * ((double)rand() / (double)(RAND_MAX + 1.0));
		k = vperm[i];
		vperm[i] = vperm[j];
		vperm[j] = k;
	}

	/* Sort vertices by degree and initialize traverse array */
	vdegavg = COARSENING_HEM_VDEGREE_LIMIT * graph->nedges
			  / nvertices;
	for (i = 0; i < nvertices; i++) {
		vdeg[i] = adjindexes[i + 1] - adjindexes[i];
		if (vdeg[i] > vdegavg)
			vdeg[i] = vdegavg;
		match[i] = -1;
	}
	if (counting_sort(vperm, vdeg, nvertices, vdegavg, traverse) > 0) {
		err = 1;
		goto errhandler;
	}

	free(vperm);
	free(vdeg);
	vdeg = NULL;
	vperm = NULL;

	/* Build matching */
	coarse_nvertices = 0;
	coarse_nedges = 0;

	/* Traverse all single vertices and join them with non single */
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];

		if (match[v] == -1) {
			if (adjindexes[v] < adjindexes[v + 1]) {
				/* Non single vertex */
				break;
			}

			vmax = v;
			for (j = nvertices - 1; j > i; j--)
			{
				k = traverse[j];
				if (match[k] == -1
					&& (adjindexes[k] < adjindexes[k + 1]))
				{
					vmax = k;
					break;
				}
			}

			/* Join vertex v and vmax into multivertex */
			graph->vmap[v] = coarse_nvertices;
			graph->vmap[vmax] = coarse_nvertices;
			match[v] = vmax;
			match[vmax] = v;
			coarse_nvertices++;
		}
	}

	/* Traverse other vertices */
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];

		if (match[v] == -1) {
			wmax = 0;
			vmax = v;

			/* Find heavy edge */
			for (j = adjindexes[v]; j < adjindexes[v + 1];
				 j++)
			{
				k = graph->adjv[j];
				if (match[k] == -1 && wmax < graph->edges[j]
				    &&  (graph->vweights[v] + graph->vweights[k])
				        <= opt->vweight_max)
				{
					vmax = k;
					wmax = graph->edges[j];
				}
			}

			/* Merge two vertices in multivertex */
			graph->vmap[v] = coarse_nvertices;
			graph->vmap[vmax] = coarse_nvertices;
			coarse_nvertices++;
			match[v] = vmax;
			match[vmax] = v;
		}
	}

	/* Allocate memory for coarse graph */
	if ( (cg = create_coarse_graph(g, coarse_nvertices,
			                       traverse, match)) == NULL)
	{
		err = 1;
		goto errhandler;
	}
	cg->prev = g;

errhandler:
	free(match);
	free(traverse);
	free(vperm);
	free(vdeg);

	if (err) {
		free(graph->vmap);
		graph->vmap = NULL;
	}

	return cg;
}

/*
 * coarsen_hem_regular: Coarsen regular graph by Heavy Edge Matching (HEM)
 *                      heuristic. Return pointer to coarse graph.
 */
coarse_graph_t *coarsen_hem_regular(coarse_graph_t *g, gpart_options_t *opt)
{
	csrgraph_t *graph;
	int nvertices, *adjindexes;
	int err = 0;
	int i, j, k, v;
	coarse_graph_t *cg = NULL;
	int *match = NULL;
	int *traverse = NULL;
	int coarse_nvertices;
	int coarse_nedges;
	int vmax, wmax;

	graph = g->graph;
	nvertices = g->graph->nvertices;
	adjindexes = g->graph->adjindexes;

	if ( (graph->vmap = malloc(sizeof(*graph->vmap) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (match = malloc(sizeof(*match) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}
	if ( (traverse = malloc(sizeof(*traverse) * nvertices)) == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Permute vertices for random traverse of graph */
	for (i = 0; i < nvertices; i++) {
		traverse[i] = i;
		match[i] = -1;
	}

	/* Build matching */
	coarse_nvertices = 0;
	coarse_nedges = 0;

	/* Traverse all single vertices and join them with non single */
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];

		if (match[v] == -1) {
			if (adjindexes[v] < adjindexes[v + 1]) {
				/* Non single vertex */
				break;
			}

			vmax = v;
			for (j = nvertices - 1; j > i; j--) {
				k = traverse[j];
				if (match[k] == -1 && (adjindexes[k] < adjindexes[k + 1])) {
					vmax = k;
					break;
				}
			}

			/* Join vertex v and vmax into multivertex */
			graph->vmap[v] = coarse_nvertices;
			graph->vmap[vmax] = coarse_nvertices;
			match[v] = vmax;
			match[vmax] = v;
			coarse_nvertices++;
		}
	}

	/* Traverse other vertices */
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];

		if (match[v] == -1) {
			wmax = 0;
			vmax = v;

			/* Find heavy edge */
			for (j = adjindexes[v]; j < adjindexes[v + 1]; j++) {
				k = graph->adjv[j];
				if (match[k] == -1 && wmax < graph->edges[j]
				    &&  (graph->vweights[v] + graph->vweights[k])
				        <= opt->vweight_max)
				{
					vmax = k;
					wmax = graph->edges[j];
				}
			}

			/* Merge two vertices in multivertex */
			graph->vmap[v] = coarse_nvertices;
			graph->vmap[vmax] = coarse_nvertices;
			coarse_nvertices++;
			match[v] = vmax;
			match[vmax] = v;
		}
	}

	/* Allocate memory for coarse graph */
	if ( (cg = create_coarse_graph_regular(g, coarse_nvertices,
			                               traverse, match)) == NULL)
	{
		err = 1;
		goto errhandler;
	}
	cg->prev = g;

errhandler:
	free(match);
	free(traverse);

	if (err) {
		free(graph->vmap);
		graph->vmap = NULL;
	}

	return cg;
}


/* create_coarse_graph: Allocate memory and fill coarse graph. */
coarse_graph_t *create_coarse_graph(coarse_graph_t *g, int coarse_nvertices,
		                            int *traverse, int *match)
{
	csrgraph_t *graph, *cggraph;
	int nvertices, *adjindexes;
	int err = 0;
	int i, j, k, z, v, u;
	int coarse_nedges;
	coarse_graph_t *cg = NULL;
	int *visited;
	int nedges;

	graph = g->graph;
	nvertices = g->graph->nvertices;
	adjindexes = g->graph->adjindexes;

	if ( (visited = malloc(sizeof(*visited) * coarse_nvertices)) == NULL)
	{
		err = 1;
		goto errhandler;
	}

	/* Allocate memory for coarse graph */
	if ( (cg = malloc(sizeof(*cg))) == NULL) {
		err = 1;
		goto errhandler;
	}
	cg->prev = g;
	cg->next = NULL;
	cg->graph = csrgraph_create(coarse_nvertices, graph->nedges / 2);
	if (cg->graph == NULL) {
		err = 1;
		goto errhandler;
	}
	cggraph = cg->graph;
	cggraph->vweights = malloc(sizeof(*cggraph->vweights) * coarse_nvertices);
	if (cggraph->vweights == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Construct coarse graph */
	for (i = 0; i < coarse_nvertices; i++)
		visited[i] = -1;
	coarse_nedges = 0;
	coarse_nvertices = 0;
	cggraph->adjindexes[0] = 0;
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];
		if (graph->vmap[v] != coarse_nvertices)
			continue;

		cggraph->vweights[coarse_nvertices] = graph->vweights[v];

		nedges = 0;
		for (j = adjindexes[v]; j < adjindexes[v + 1];
			 j++)
		{
			z = graph->vmap[graph->adjv[j]];
			if ( (k = visited[z]) == -1) {
				cggraph->adjv[coarse_nedges + nedges] = z;
				cggraph->edges[coarse_nedges + nedges] = graph->edges[j];
				visited[z] = nedges++;
			} else {
				cggraph->edges[coarse_nedges + k] += graph->edges[j];
			}
		}

		u = match[v];
		if (v != u) {
			cggraph->vweights[coarse_nvertices] += graph->vweights[u];

			for (j = adjindexes[u]; j < adjindexes[u + 1];
				 j++)
			{
				z = graph->vmap[graph->adjv[j]];
				if ( (k = visited[z]) == -1) {
					cggraph->adjv[coarse_nedges + nedges] = z;
					cggraph->edges[coarse_nedges + nedges] = graph->edges[j];
					visited[z] = nedges++;
				} else {
					cggraph->edges[coarse_nedges + k] += graph->edges[j];
				}
			}
		}

		if ( (j = visited[coarse_nvertices]) != -1) {
			/* Remove internal edge */
			nedges--;
			cggraph->adjv[coarse_nedges + j] =
					cggraph->adjv[coarse_nedges + nedges];
			cggraph->edges[coarse_nedges + j] =
					cggraph->edges[coarse_nedges + nedges];
			visited[coarse_nvertices] = -1;
		}

		for (j = 0; j < nedges; j++)
			visited[cggraph->adjv[coarse_nedges + j]] = -1;

		coarse_nedges += nedges;
		coarse_nvertices++;
		cggraph->adjindexes[coarse_nvertices] = coarse_nedges;
	}
	cggraph->adjindexes[coarse_nvertices] = coarse_nedges;

	/* Reallocate memory for real number of edges in coarse graph */
	cggraph->nedges = coarse_nedges;
	cggraph->adjv = realloc(cggraph->adjv,
			                sizeof(*cggraph->adjv) * (coarse_nedges + 1));
	cggraph->edges = realloc(cggraph->edges,
			                 sizeof(*cggraph->edges) * (coarse_nedges + 1));
	if (cggraph->adjv == NULL || cggraph->edges == NULL) {
		err = 1;
		goto errhandler;
	}
	cggraph->nedges = coarse_nedges;

errhandler:
	if (err) {
		if (cg) {
			if (cg->graph) {
				if (cg->graph->vweights)
					free(cg->graph->vweights);
				csrgraph_free(cg->graph);
			}
			free(cg);
			cg = NULL;
		}
	}
	free(visited);
	return cg;
}

/* create_coarse_graph_regular: Allocate memory and fill coarse graph. */
coarse_graph_t *create_coarse_graph_regular(coarse_graph_t *g,
		                                    int coarse_nvertices,
		                                    int *traverse, int *match)
{
	csrgraph_t *graph, *cggraph;
	int nvertices, *adjindexes;
	int err = 0;
	int i, j, k, z, v, u;
	int coarse_nedges;
	coarse_graph_t *cg = NULL;
	int *visited;
	int nedges;

	graph = g->graph;
	nvertices = g->graph->nvertices;
	adjindexes = g->graph->adjindexes;

	if ( (visited = malloc(sizeof(*visited) * coarse_nvertices)) == NULL)
	{
		err = 1;
		goto errhandler;
	}

	/* Allocate memory for coarse graph */
	if ( (cg = malloc(sizeof(*cg))) == NULL) {
		err = 1;
		goto errhandler;
	}
	cg->prev = g;
	cg->next = NULL;
	cg->graph = csrgraph_create(coarse_nvertices, graph->nedges / 2);
	if (cg->graph == NULL) {
		err = 1;
		goto errhandler;
	}
	cggraph = cg->graph;
	cggraph->vweights = malloc(sizeof(*cggraph->vweights) * coarse_nvertices);
	if (cggraph->vweights == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Construct coarse graph */
	for (i = 0; i < coarse_nvertices; i++)
		visited[i] = -1;
	coarse_nedges = 0;
	coarse_nvertices = 0;
	cggraph->adjindexes[0] = 0;
	for (i = 0; i < nvertices; i++) {
		v = traverse[i];
		if (graph->vmap[v] != coarse_nvertices)
			continue;

		cggraph->vweights[coarse_nvertices] = graph->vweights[v];

		nedges = 0;
		for (j = adjindexes[v]; j < adjindexes[v + 1]; j++) {
			z = graph->vmap[graph->adjv[j]];
			if ( (k = visited[z]) == -1) {
				cggraph->adjv[coarse_nedges + nedges] = z;
				cggraph->edges[coarse_nedges + nedges] = graph->edges[j];
				visited[z] = nedges++;
			} else {
				cggraph->edges[coarse_nedges + k] += graph->edges[j];
			}
		}

		u = match[v];
		if (v != u) {
			cggraph->vweights[coarse_nvertices] += graph->vweights[u];

			for (j = adjindexes[u]; j < adjindexes[u + 1]; j++) {
				z = graph->vmap[graph->adjv[j]];
				if ( (k = visited[z]) == -1) {
					cggraph->adjv[coarse_nedges + nedges] = z;
					cggraph->edges[coarse_nedges + nedges] = graph->edges[j];
					visited[z] = nedges++;
				} else {
					cggraph->edges[coarse_nedges + k] += graph->edges[j];
				}
			}
		}

		if ( (j = visited[coarse_nvertices]) != -1) {
			/* Remove internal edge */
			nedges--;
			cggraph->adjv[coarse_nedges + j] =
					cggraph->adjv[coarse_nedges + nedges];
			cggraph->edges[coarse_nedges + j] =
					cggraph->edges[coarse_nedges + nedges];
			visited[coarse_nvertices] = -1;
		}

		for (j = 0; j < nedges; j++)
			visited[cggraph->adjv[coarse_nedges + j]] = -1;

		coarse_nedges += nedges;
		coarse_nvertices++;
		cggraph->adjindexes[coarse_nvertices] = coarse_nedges;
	}
	cggraph->adjindexes[coarse_nvertices] = coarse_nedges;

	/* Reallocate memory for real number of edges in coarse graph */
	cggraph->nedges = coarse_nedges;
	cggraph->adjv = realloc(cggraph->adjv,
			                sizeof(*cggraph->adjv) * (coarse_nedges + 1));
	cggraph->edges = realloc(cggraph->edges,
			                 sizeof(*cggraph->edges) * (coarse_nedges + 1));
	if (cggraph->adjv == NULL || cggraph->edges == NULL) {
		err = 1;
		goto errhandler;
	}
	cggraph->nedges = coarse_nedges;

errhandler:
	if (err) {
		if (cg) {
			if (cg->graph) {
				if (cg->graph->vweights)
					free(cg->graph->vweights);
				csrgraph_free(cg->graph);
			}
			free(cg);
			cg = NULL;
		}
	}
	free(visited);
	return cg;
}

/*
 * gpart_project_bisection: Project bisection to source graph and refine it.
 *                          Return 0 on success and 1 otherwise.
 */
int gpart_project_bisection(coarse_graph_t *cg, csrgraph_t *srcgraph,
		                    int *bisection, gpart_options_t *opt,
		                    int *partsizes)
{
	int i, n;
	coarse_graph_t *p = NULL;
	int *bisection_coarse = NULL;
	int coarse_nvertices;
	edgecut_t edgecut;

	if (cg->prev == NULL) {
		/* List is empty. Free head. */
		free(cg);
		return 0;
	}

	n = srcgraph->nvertices;
	if ( (bisection_coarse = malloc(sizeof(*bisection_coarse) * n)) == NULL)
		return 1;

	coarse_nvertices = cg->graph->nvertices;
	for (cg = cg->prev; cg != NULL; ) {

		for (i = 0; i < coarse_nvertices; i++)
			bisection_coarse[i] = bisection[i];

		/* Project bisection and refine */
		for (i = 0; i < cg->graph->nvertices; i++)
			bisection[i] = bisection_coarse[cg->graph->vmap[i]];

		gpart_refine_bisection_fm(cg->graph, bisection, partsizes, &edgecut);

		coarse_nvertices = cg->graph->nvertices;
		p = cg;
		cg = cg->prev;

		/* Free graph memory */
		if (p->graph != srcgraph) {
			csrgraph_free(p->graph);
			free(p);
		}
	}

	free(bisection_coarse);
	return 0;
}

/*
 * gpart_project_bisection_regular: Project bisection to source regular graph
 *                                  and refine it.
 *                                  Return 0 on success and 1 otherwise.
 */
int gpart_project_bisection_regular(coarse_graph_t *cg, csrgraph_t *srcgraph,
		                            int *bisection, gpart_options_t *opt,
		                            int *partsizes)
{
	int i, n;
	coarse_graph_t *p = NULL;
	int *bisection_coarse = NULL;
	int coarse_nvertices;
	edgecut_t edgecut;

	if (cg->prev == NULL) {
		/* List is empty. Free head. */
		free(cg);
		return 0;
	}

	n = srcgraph->nvertices;
	if ( (bisection_coarse = malloc(sizeof(*bisection_coarse) * n)) == NULL)
		return 1;

	coarse_nvertices = cg->graph->nvertices;
	for (cg = cg->prev; cg != NULL; ) {

		for (i = 0; i < coarse_nvertices; i++)
			bisection_coarse[i] = bisection[i];

		/* Project bisection and refine */
		for (i = 0; i < cg->graph->nvertices; i++)
			bisection[i] = bisection_coarse[cg->graph->vmap[i]];

		gpart_refine_bisection_fm(cg->graph, bisection, partsizes, &edgecut);

		coarse_nvertices = cg->graph->nvertices;
		p = cg;
		cg = cg->prev;

		/* Free graph memory */
		if (p->graph != srcgraph) {
			csrgraph_free(p->graph);
			free(p);
		}
	}

	free(bisection_coarse);
	return 0;
}

/* counting_sort: Counting sort of items in src array by values. */
int counting_sort(int *src, int *values, int n, int valmax, int *dst)
{
	int i, j;
	int *c = NULL;

	if ( (c = malloc(sizeof(*c) * (valmax + 2))) == NULL)
		return 1;

	for (i = 0; i <= valmax; i++)
		c[i] = 0;
	for (i = 0; i < n; i++)
		c[values[i]]++;

	for (i = 1; i <= valmax; i++)
		c[i] += c[i - 1];
	for (i = valmax + 1; i > 0; i--)
		c[i] = c[i - 1];
	c[0] = 0;

	for (i = 0; i < n; i++) {
		j = src[i];
		dst[c[values[j]]++] = j;
	}

	free(c);
	return 0;
}
