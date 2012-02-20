/*
 * csrgraph.h: Compressed Sparse Row (CSR) graph format routines.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "gpart.h"

#define SETERR(v, n) v |= (1 << n)

enum {
	LINESIZE_MAX = 64 * 1024
};

struct csrgraph_adjvertex {
	int v, w;
};

static int csrgraph_adjvertex_cmp(const void *a, const void *b);

/* csrgraph_create: Allocate memory for graph. */
csrgraph_t *csrgraph_create(int nvertices, int nedges)
{
	int err = 0;
	csrgraph_t *graph = NULL;

	if ( (graph = malloc(sizeof(*graph))) == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	graph->vparents = NULL;
	graph->vweights = NULL;
	graph->vmap = NULL;

	graph->nvertices = nvertices;
	graph->nedges = nedges;

	/* Allocate memory for graph */
	graph->adjindexes = malloc(sizeof(int) * (graph->nvertices + 1));
	if (graph->adjindexes == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}

	/* Allocate memory for weights of edges */
	graph->nedges *= 2;
	graph->edges = malloc(sizeof(int) * (graph->nedges + 1));
	if (graph->edges == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}

	graph->adjv = malloc(sizeof(int) * (graph->nedges + 1));
	if (graph->adjv == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}

errhandler:
	if (err) {
		if (graph) {
			free(graph->adjindexes);
			free(graph->adjv);
			free(graph->edges);
			free(graph);
			graph = NULL;
		}
	}
	return graph;
}

/* csrgraph_load: Read graph in CSR format from file. */
csrgraph_t *csrgraph_load(const char *filename)
{
	int err = 0;
	FILE *fin = NULL;
	csrgraph_t *graph = NULL;
	char *line = NULL, *buf = NULL, *ptr = NULL;
	int i, j, v, w;
	
	fprintf(stderr,"load graph step 1\n");
	if ( (fin = fopen(filename, "r")) == NULL) {
	        fprintf(stderr, "Cant' load graph file\n");
		SETERR(err, 1);
		goto errhandler;
	}
	fprintf(stderr,"load graph step 2\n");
	if ( (graph = malloc(sizeof(*graph))) == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	graph->vparents = NULL;
	graph->vweights = NULL;
	graph->vmap = NULL;
        fprintf(stderr,"load graph step 3\n");

	if ( (buf = malloc(sizeof(*buf) * LINESIZE_MAX)) == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	fprintf(stderr,"load graph step 4\n");
	if (fscanf(fin, "%d %d %d\n", &graph->nvertices, 
		                          &graph->nedges, &i) != 3) 
	{
		SETERR(err, 1);
		goto errhandler;
	}
        fprintf(stderr,"load graph step 5\n");
	/* Allocate memory for graph */
	graph->adjindexes = malloc(sizeof(int) * (graph->nvertices + 1));
	if (graph->adjindexes == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	fprintf(stderr,"load graph step 6\n");
	/* Allocate memory for weights of edges */
	graph->nedges *= 2;
	graph->edges = malloc(sizeof(int) * graph->nedges);
	if (graph->edges == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	fprintf(stderr,"load graph step 7\n");
	graph->adjv = malloc(sizeof(int) * graph->nedges);
	if (graph->adjv == NULL) {
		SETERR(err, 1);
		goto errhandler;
	}
	fprintf(stderr,"load graph step 8\n");
	for (i = 0, j = 0; i < graph->nvertices; i++) {
		
		if (fgets(buf, LINESIZE_MAX, fin) == NULL) {
			SETERR(err, 1);
			goto errhandler;
		}

		buf[strlen(buf) - 1] = '\0';
		if (strlen(buf) > 0) {
			line = buf;
			
			/* Read adj. list of vertex i */
			graph->adjindexes[i] = j;
			do {			
				errno = 0;
				v = (int)strtol(buf, &ptr, 10);
				if (errno != 0) {
					SETERR(err, 1);
					goto errhandler;				
				}
				buf = ptr;

				errno = 0;
				w = strtol(buf, &ptr, 10);
				if (errno != 0) {
					SETERR(err, 1);
					goto errhandler;				
				}
				buf = ptr;
				
				graph->adjv[j] = v - 1;
				graph->edges[j] = w;
				j++;
				
				while (*buf == ' ')
					buf++;
			} while (*buf != '\0');
			
			buf = line;
		}
	}	
	graph->adjindexes[graph->nvertices] = graph->nedges;
	fprintf(stderr,"load graph step 9\n");
errhandler:
	if (fin)
		fclose(fin);
	free(buf);
	
	if (err) {
		if (graph) { 
			free(graph->adjindexes);
			free(graph->adjv);
			free(graph->edges);
			free(graph);
			graph = NULL;
		}
	}
	return graph;
}

/* csrgraph_save: Save graph in CSR format. */
int csrgraph_save(csrgraph_t *g, const char *filename)
{
	int i, j;
	FILE *fout;

	if (g == NULL)
		return 1;

	if ( (fout = fopen(filename, "w")) == NULL)
		return 1;

	fprintf(fout, "%d %d 1\n", g->nvertices, g->nedges / 2);
	for (i = 0; i < g->nvertices; i++) {
		for (j = g->adjindexes[i]; j < g->adjindexes[i + 1]; j++)
			fprintf(fout, "%d %d ", g->adjv[j] + 1, g->edges[j]);
		fprintf(fout, "\n");
	}

	fclose(fout);
	return 0;
}

/* csrgraph_free: Destroy graph. */
void csrgraph_free(csrgraph_t *graph)
{
	if (graph) {
		free(graph->adjindexes);
		free(graph->adjv);
		free(graph->edges);
		if (graph->vparents);
			free(graph->vparents);
		if (graph->vweights)
			free(graph->vweights);
		if (graph->vmap)
			free(graph->vmap);
	}
}

/* csrgraph_get_edge_weight: Return weight of edge (i, j) or 0; i > 0, j > 0. */
int csrgraph_get_edge_weight(csrgraph_t *g, int i, int j)
{
	int k;

	for (k = g->adjindexes[i - 1]; k < g->adjindexes[i]; k++) {
		if (g->adjv[k] == (j - 1))
			return g->edges[k];
	}
	return 0;
}

/* csrgraph_bisect: Bisect graph on two subgraphs. */
int csrgraph_bisect(csrgraph_t *graph, int *bisection,
		            csrgraph_t **leftgraph, csrgraph_t **rightgraph)
{
	int graphnvertices, *adjindexes, *adjv;
	int err = 0, i, j, b;
	int nvertices[2] = {0, 0};
	int nedges[2] = {0, 0};
	int curvertex[2] = {0, 0};
	int adjindex[2] = {0, 0};
	int *old2new_vertices[2] = { NULL, NULL };
	csrgraph_t *g = NULL;

	if (graph->nvertices < 2 || graph->nedges == 0)
		return 1;

	graphnvertices = graph->nvertices;
	adjindexes = graph->adjindexes;
	adjv = graph->adjv;

	/* Arrays for translating numbers of vertices */
	old2new_vertices[0] = malloc(sizeof(int) * graphnvertices);
	old2new_vertices[1] = malloc(sizeof(int) * graphnvertices);
	if (old2new_vertices[0] == NULL || old2new_vertices[1] == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Compute number of vertices and edges in subgraphs */
	for (i = 0; i < graphnvertices; i++) {
		old2new_vertices[bisection[i]][i] = nvertices[bisection[i]];
		nvertices[bisection[i]]++;
		for (j = adjindexes[i]; j < adjindexes[i + 1]; j++) {
			if (bisection[i] == bisection[adjv[j]])
				nedges[bisection[i]]++;
		}
	}
	nedges[0] /= 2;
	nedges[1] /= 2;

	/* Create empty graphs */
	*leftgraph = csrgraph_create(nvertices[0], nedges[0]);
	*rightgraph = csrgraph_create(nvertices[1], nedges[1]);
	if (*leftgraph == NULL || *rightgraph == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Create map of bisection vertices to parent vertices */
	(*leftgraph)->vparents = malloc(sizeof(int) * (*leftgraph)->nvertices);
	(*rightgraph)->vparents = malloc(sizeof(int) * (*rightgraph)->nvertices);
	if ((*leftgraph)->vparents == NULL || (*rightgraph)->vparents == NULL) {
		err = 1;
		goto errhandler;
	}

	for (i = 0; i < graphnvertices; i++) {
		g = (bisection[i] == 0) ? *leftgraph : *rightgraph;
		b = bisection[i];

		if (graph->vparents)
			g->vparents[curvertex[b]] = graph->vparents[i];
		else
			g->vparents[curvertex[b]] = i;
		g->adjindexes[curvertex[b]] = adjindex[b];

		for (j = adjindexes[i]; j < adjindexes[i + 1]; j++) {
			if (b == bisection[adjv[j]]) {
				/* If adj vertex in same subset as i */
				g->adjv[adjindex[b]] = old2new_vertices[b][adjv[j]];
				g->edges[adjindex[b]] = graph->edges[j];
				adjindex[b]++;
			}
		}
		curvertex[b]++;
	}
	(*leftgraph)->adjindexes[(*leftgraph)->nvertices] = (*leftgraph)->nedges;
	(*rightgraph)->adjindexes[(*rightgraph)->nvertices] = (*rightgraph)->nedges;

errhandler:
	free(old2new_vertices[0]);
	free(old2new_vertices[1]);

	if (err) {
		csrgraph_free(*leftgraph);
		csrgraph_free(*rightgraph);
		return 1;
	}
	return 0;
}

/* csrgraph_print_adjmatrix: */
void csrgraph_print_adjmatrix(csrgraph_t *graph)
{
	int i, j;

	if (graph == NULL)
		return;

	for (i = 0; i < graph->nvertices; i++) {
		printf("%d: ", i);
		for (j = graph->adjindexes[i]; j < graph->adjindexes[i + 1]; j++) {
			printf("%d/%d ", graph->adjv[j], graph->edges[j]);
		}
		printf("\n");
	}
}

/* csrgraph_adjvertex_cmp: */
int csrgraph_adjvertex_cmp(const void *a, const void *b)
{
	if (((struct csrgraph_adjvertex *)a)->v ==
		((struct csrgraph_adjvertex *)b)->v)
	{
		return 0;
	} else {
		return (((struct csrgraph_adjvertex *)a)->v <
				((struct csrgraph_adjvertex *)b)->v) ? -1 : 1;
	}
}

/* csrgraph_generate_bruck: Generates task graph of Bruck's algorithm. */
csrgraph_t *csrgraph_generate_bruck(int nvertices)
{
	csrgraph_t *g = NULL;
	int i, j, k, m, r, nsteps, deg, nedges;
	int rsend, rrecv, pof2, nadjmax;
	struct csrgraph_adjvertex *adj = NULL;

	if (nvertices < 2)
		return NULL;

	/* Compute degree of vertices and number of edges */
	nsteps = ceil(log(nvertices) / log(2.0));
	nadjmax = 2 * nsteps;
	if ( (adj = malloc(sizeof(*adj) * nadjmax)) == NULL)
		return NULL;
	for (i = 0; i < nadjmax; i++)
		adj[i].v = -1;

	for (pof2 = 1, j = 0, k = 0; k < nsteps; k++, pof2 *= 2) {
		rsend = (nvertices - pof2) % nvertices;
		rrecv = pof2 % nvertices;
		adj[j++].v = rsend;
		adj[j++].v = rrecv;
	}
	qsort(adj, nadjmax, sizeof(*adj), csrgraph_adjvertex_cmp);
	for (i = 0; adj[i].v == -1 && i < nadjmax; i++)
		;
	for (deg = 1, i++; i < nadjmax; i++) {
		if (adj[i].v != adj[i - 1].v)
			deg++;
	}
	nedges = deg * nvertices / 2;

	/* Allocate memory for graph */
	if ( (g = csrgraph_create(nvertices, nedges)) == NULL) {
		free(adj);
		return NULL;
	}

	/* Setup adjacency lists */
	g->adjindexes[0] = 0;
	for (i = 1; i < nvertices; i++)
		g->adjindexes[i] = g->adjindexes[i - 1] + deg;
	g->adjindexes[nvertices] = 2 * nedges;

	/* Fill adjacency lists */
	for (r = 0; r < nvertices; r++) {

		/* Fill list of process i */
		for (pof2 = 1, j = 0; pof2 <= nvertices / 2; pof2 *= 2) {
			adj[j].v = (nvertices + r - pof2) % nvertices;
			adj[j++].w = pof2;
			adj[j].v = (r + pof2) % nvertices;
			adj[j++].w = pof2;
		}
		if (nvertices - pof2 > 0) {
			m = nvertices - pof2;
			adj[j].v = (nvertices + r - pof2) % nvertices;
			adj[j++].w = m;
			adj[j].v = (r + pof2) % nvertices;
			adj[j++].w = m;
		}

		qsort(adj, nadjmax, sizeof(*adj), csrgraph_adjvertex_cmp);
		for (k = 0, i = 0; i < nadjmax; ) {
			g->adjv[g->adjindexes[r] + k] = adj[i].v;
			for (m = 0, j = adj[i].v; adj[i].v == j && i < nadjmax; i++)
				m += adj[i].w;
			g->edges[g->adjindexes[r] + k] = m;
			k++;
		}
	}
	free(adj);
	return g;
}

/*
 * csrgraph_generate_recdoubling: Generates task graph of recursive doubling
 *                                algorithm.
 */
csrgraph_t *csrgraph_generate_recdoubling(int nvertices)
{
	csrgraph_t *g = NULL;
	int i, k, r, nsteps, nedges;
	int pof2;

	if (nvertices < 2)
		return NULL;

	/* Compute degree of vertices and number of edges */
	nsteps = ceil(log(nvertices) / log(2.0));
	nedges = nsteps * nvertices / 2;

	/* Construct graph */
	if ( (g = csrgraph_create(nvertices, nedges)) == NULL)
		return NULL;

	/* Setup adjacency lists */
	g->adjindexes[0] = 0;
	for (i = 1; i < nvertices; i++)
		g->adjindexes[i] = g->adjindexes[i - 1] + nsteps;
	g->adjindexes[nvertices] = 2 * nedges;

	/* Fill adjacency lists */
	for (r = 0; r < nvertices; r++) {
		/* Fill list of process i */
		k = g->adjindexes[r];
		for (pof2 = 1, i = 0; pof2 < nvertices; pof2 *= 2, i++) {
			g->adjv[k + i] = r ^ pof2;
			g->edges[k + i] = 2 * pof2;
		}
	}
	return g;
}
