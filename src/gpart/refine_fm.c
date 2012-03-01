/*
 * refine_fm.c: Fiduccia-Mattheyses heuristic [1, 2] for refine bisections.
 *
 * [1] C.M. Fiduccia and R. M. Mattheyses. A linear-time heuristic for improving
 *     network partitions // Proc. of Conference "Design Automation",
 *     1982. – P. 175-181.
 * [2] G. Karypis and V. Kumar. A fast and high quality multilevel scheme for
 *     partitioning irregular graphs // SIAM Journal on Scientific Computing,
 *     1999, Vol. 20, No. 1, P.359-392.
 *
 * (C) 2007-2010 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "refine_fm.h"
#include "gpart.h"

#define FM_MOVES_LIMIT      0.15
#define FM_MOVES_LIMIT_MIN  20
#define FM_MOVES_LIMIT_MAX  100

#define FM_VWAVG_SCALE  0.05

#define FM_HASHTABLE_GAIN_LIMIT      512
#define FM_HASHTABLE_VERTICES_LIMIT  512

#define FM_ITERS_MAX  4

#define FM_PQUEUE_HASHTABLE  1
#define FM_PQUEUE_HEAP       2

typedef int gain_t;					/* Gain is signed int! */

/* Hash table */
typedef struct fm_hashtab_vertex fm_hashtab_vertex_t;
struct fm_hashtab_vertex {
	int v;							/* Vertex number */
	fm_hashtab_vertex_t *next;		/* Next vertex in list with same gain */
};

typedef struct fm_hashtab fm_hashtab_t;
struct fm_hashtab {
	int size;						/* Number of items in hash table */
	int nvertices;
	fm_hashtab_vertex_t **hashtab;
	fm_hashtab_vertex_t *vertices;	/* List of vertices */
	gain_t gain_max;				/* Maximal value of gain */
	gain_t gain_max_ptr;			/* Pointer to max. gain item in hashtab */
};

/* Heap */
typedef struct fm_heap_node fm_heap_node_t;
struct fm_heap_node {
	int key;
	gain_t value;
};

typedef struct fm_heap fm_heap_t;
struct fm_heap {
	int nvertices;
	int nvertices_max;
	fm_heap_node_t *heap;
	int *pointer;
};

/* Priority queue */
typedef struct fm_pqueue fm_pqueue_t;
struct fm_pqueue {
	int type;
	fm_hashtab_t *hashtab;
	fm_heap_t *heap;
};

static fm_pqueue_t *fm_pqueue_create(csrgraph_t *g, gain_t gainmax);
static void fm_pqueue_free(fm_pqueue_t *q);
static void fm_pqueue_clear(fm_pqueue_t *q);
static void fm_pqueue_insert(fm_pqueue_t *q, int v, gain_t gain);
static int fm_pqueue_get_maxgain_vertex(fm_pqueue_t *q);
static void fm_pqueue_delete(fm_pqueue_t *q, int v, gain_t gain);
static void fm_pqueue_update(fm_pqueue_t *q, int v, gain_t gainold,
		                     gain_t gainnew);

static fm_heap_t *fm_heap_create(csrgraph_t *g);
static void fm_heap_free(fm_heap_t *h);
static void fm_heap_clear(fm_heap_t *h);
static void fm_heap_insert(fm_heap_t *h, int v, gain_t gain);
static int fm_heap_get_maxgain_vertex(fm_heap_t *h);
static void fm_heap_delete(fm_heap_t *h, int v, gain_t gain);
static void fm_heap_update(fm_heap_t *h, int v, gain_t gainold, int gainnew);

static fm_hashtab_t *fm_hashtab_create(csrgraph_t *g, gain_t gainmax);
static void fm_hashtab_free(fm_hashtab_t *b);
static void fm_hashtab_clear(fm_hashtab_t *b);
static void fm_hashtab_insert(fm_hashtab_t *b, int v, gain_t gain);
static void fm_hashtab_update(fm_hashtab_t *b, int v, gain_t gainold,
		                     gain_t gainnew);
static void fm_hashtab_delete(fm_hashtab_t *b, int v, gain_t gain);
static int fm_hashtab_get_maxgain_vertex(fm_hashtab_t *b);

/*
 * gpart_refine_bisection_fm: Refine bisection by Fiduccia-Mattheyses heuristic.
 *                            Return edge-cut in edgecut.
 *                            Return 0 on success and 1 otherwise.
 */
int gpart_refine_bisection_fm(csrgraph_t *g, int *bisection, int *partsizes,
		                      edgecut_t *edgecut)
{
	int nvertices, *adjindexes;
	int i, j, tmp, v, sum, iter, err = 0;
	int srcpart, dstpart;
	fm_pqueue_t *pqueues[2] = { NULL, NULL };
	gain_t *extcosts = NULL, *intcosts = NULL, extcost_old, intcost_old,
		   gain_old, gainmax, edgecut_cur, edgecut_best, gain;
	int *moves = NULL, *vertices_moves = NULL;
	int moveno, move_limit, edgecut_best_moveno;
	int pw[2], pwdiff, pwdiff_min, vwavg;
	int *perm = NULL;

	nvertices = g->nvertices;
	adjindexes = g->adjindexes;

	if ( (extcosts = calloc(nvertices, sizeof(*extcosts))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (intcosts = calloc(nvertices, sizeof(*intcosts))) == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Compute gains and initial edgecut */
	*edgecut = 0;
	gainmax = 0;
	for (i = 0; i < nvertices; i++) {
		extcosts[i] = 0;
		intcosts[i] = 0;
		gain = 0;
		for (j = adjindexes[i]; j < adjindexes[i + 1]; j++) {
			if (bisection[g->adjv[j]] != bisection[i])
				extcosts[i] += g->edges[j];      /* External edge */
			else
				intcosts[i] += g->edges[j];      /* Internal edge */
			gain += g->edges[j];
		}
		*edgecut += extcosts[i];
		if (gain > gainmax)
			gainmax = gain;
	}
	*edgecut /= 2;

	if ( (pqueues[0] = fm_pqueue_create(g, gainmax)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (pqueues[1] = fm_pqueue_create(g, gainmax)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (moves = calloc(nvertices, sizeof(*moves))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (perm = calloc(nvertices, sizeof(*perm))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (vertices_moves = calloc(nvertices,
			                      sizeof(*vertices_moves))) == NULL)
	{
		err = 1;
		goto errhandler;
	}

	/* Setup limit on vertices moves without edge-cut decreasing */
	move_limit = nvertices * FM_MOVES_LIMIT;
	if (move_limit < FM_MOVES_LIMIT_MIN)
		move_limit = FM_MOVES_LIMIT_MIN;
	if (move_limit > FM_MOVES_LIMIT_MAX)
		move_limit = FM_MOVES_LIMIT_MAX;

	/* Set limits on size of partitions */
	pw[0] = 0;
	for (sum = 0, i = 0; i < nvertices; i++) {
		sum += g->vweights[i];
		if (bisection[i] == 0)
			pw[0] += g->vweights[i];
	}
	pw[1] = sum - pw[0];
	pwdiff = abs(partsizes[0] - pw[0]);

	vwavg = 2 * sum / nvertices;
	tmp = sum * FM_VWAVG_SCALE;
	if (vwavg > tmp)
		vwavg = tmp;

	edgecut_best = *edgecut;
	for (i = 0; i < nvertices; i++)
		vertices_moves[i] = -1;

	/* Iterations of FM heuristic */
	for (iter = 0; iter < FM_ITERS_MAX; iter++) {

		edgecut_best_moveno = -1;
		edgecut_cur = edgecut_best;
		pwdiff_min = abs(partsizes[0] - pw[0]);

		fm_pqueue_clear(pqueues[0]);
		fm_pqueue_clear(pqueues[1]);

		/* Permute boundary vertices and insert them in queues. */
	    for (i = 0; i < nvertices; i++)
	    	perm[i] = i;
	    for (i = 0; i < nvertices; i++) {
	    	j = g->nvertices * ((double)rand() / (double)(RAND_MAX + 1.0));
	    	tmp = perm[i];
	    	perm[i] = perm[j];
	    	perm[j] = tmp;
	    }

		for (i = 0; i < nvertices; i++) {
			j = perm[i];
			if (extcosts[j] > 0 || adjindexes[i] == adjindexes[i + 1]) {
				fm_pqueue_insert(pqueues[bisection[j]], j,
						         extcosts[j] - intcosts[j]);
			}
		}

		/* Move vertices from one partition to another */
		for (moveno = 0; moveno < nvertices; moveno++) {

			srcpart = (partsizes[0] - pw[0] < partsizes[1] - pw[1])
					  ? 0 : 1;
			dstpart = (srcpart + 1) % 2;

			/* Find in partition srcpart vertex with max gain */
			if ( (v = fm_pqueue_get_maxgain_vertex(pqueues[srcpart])) == -1) {
				break;
			}

			/* Move vertex v to partition dstpart */
			edgecut_cur -= (extcosts[v] - intcosts[v]);
			pw[dstpart] += g->vweights[v];
			pw[srcpart] -= g->vweights[v];

			if ((edgecut_cur < edgecut_best
			     && abs(partsizes[0] - pw[0]) <= pwdiff + vwavg)
			    || (edgecut_cur == edgecut_best
			     && abs(partsizes[0] - pw[0]) < pwdiff_min))
			{
				edgecut_best = edgecut_cur;
				edgecut_best_moveno = moveno;
				pwdiff_min = abs(partsizes[0] - pw[0]);
			} else if (moveno - edgecut_best_moveno > move_limit) {
				edgecut_cur += (extcosts[v] - intcosts[v]);
				pw[srcpart] += g->vweights[v];
				pw[dstpart] -= g->vweights[v];
				break;
			}

			bisection[v] = dstpart;
			moves[moveno] = v;
			vertices_moves[v] = moveno;

			/* Update gain of moved vertex */
			gain = extcosts[v];
			extcosts[v] = intcosts[v];
			intcosts[v] = gain;

			/* Update gains of adj vertices */
			for (i = adjindexes[v]; i < adjindexes[v + 1]; i++) {
				j = g->adjv[i];
				gain_old = extcosts[j] - intcosts[j];
				extcost_old = extcosts[j];
				intcost_old = intcosts[j];

				/* Update gain of vertex j */
				tmp = (dstpart == bisection[j]) ? g->edges[i] : -g->edges[i];
				intcosts[j] += tmp;
				extcosts[j] -= tmp;

				if (extcost_old > 0) {
					/* Vertex j was a boundary vertex */
					if (extcosts[j] == 0) {
						if (vertices_moves[j] == -1) {
							fm_pqueue_delete(pqueues[bisection[j]], j, gain_old);
						}
					} else {
						if (vertices_moves[j] == -1) {
							fm_pqueue_update(pqueues[bisection[j]], j,
									         gain_old,
									         extcosts[j] - intcosts[j]);
						}
					}
				} else {
					if (extcosts[j] > 0) {
						/* Vertex j is a boundary vertex now */
						if (vertices_moves[j] == -1) {
							fm_pqueue_insert(pqueues[bisection[j]], j,
									         extcosts[j] - intcosts[j]);
						}
					}
				}
			}

		} /* for moves */

		/* Undo all moves to best move */
		for (i = 0; i < moveno; i++)
			vertices_moves[moves[i]] = -1;
		for (moveno--; moveno > edgecut_best_moveno; moveno--) {
			v = moves[moveno];
			dstpart = (bisection[v] + 1) % 2;
			bisection[v] = dstpart;

			tmp = extcosts[v];
			extcosts[v] = intcosts[v];
			intcosts[v] = tmp;

			pw[dstpart] += g->vweights[v];
			pw[(dstpart + 1) % 2] -= g->vweights[v];
			for (i = adjindexes[v]; i < adjindexes[v + 1]; i++) {
				j = g->adjv[i];
				/* Update gain of vertex j */
				if (dstpart == bisection[j]) {
					/* Internal edge of j */
					intcosts[j] += g->edges[i];
					extcosts[j] -= g->edges[i];
				} else {
					/* External edge of j */
					intcosts[j] -= g->edges[i];
					extcosts[j] += g->edges[i];
				}
			}
		} /* for undo */

		if (edgecut_best_moveno == -1 || edgecut_best == *edgecut)
			break;

	} /* for FM iterations */

	*edgecut = edgecut_best;

errhandler:
	free(extcosts);
	free(intcosts);
	free(moves);
	free(vertices_moves);
	free(perm);

	fm_pqueue_free(pqueues[0]);
	fm_pqueue_free(pqueues[1]);

	return err;
}

/*
 * gpart_balance_bisection_fm: Balance bisection.
 */
int gpart_balance_bisection_fm(csrgraph_t *g, int *bisection, int nmoves,
		                       int moveto)
{
	int i, j, tmp, v, iter, err = 0;
	int srcpart, dstpart;
	fm_pqueue_t *pqueues[2] = { NULL, NULL };
	gain_t *extcosts = NULL, *intcosts = NULL, extcost_old, intcost_old,
		   gain_old, gainmax, gain;
	int *moves = NULL, *vertices_moves = NULL;
	int moveno;
	int *perm = NULL;

	if ( (extcosts = calloc(g->nvertices, sizeof(*extcosts))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (intcosts = calloc(g->nvertices, sizeof(*intcosts))) == NULL) {
		err = 1;
		goto errhandler;
	}

	/* Compute gains */
	gainmax = 0;
	for (i = 0; i < g->nvertices; i++) {
		extcosts[i] = 0;
		intcosts[i] = 0;
		gain = 0;
		for (j = g->adjindexes[i]; j < g->adjindexes[i + 1]; j++) {
			if (bisection[g->adjv[j]] != bisection[i])
				extcosts[i] += g->edges[j];      /* External edge */
			else
				intcosts[i] += g->edges[j];      /* Internal edge */
			gain += g->edges[j];
		}
		if (gain > gainmax)
			gainmax = gain;
	}

	if ( (pqueues[0] = fm_pqueue_create(g, gainmax)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (pqueues[1] = fm_pqueue_create(g, gainmax)) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (moves = calloc(g->nvertices, sizeof(*moves))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (perm = calloc(g->nvertices, sizeof(*perm))) == NULL) {
		err = 1;
		goto errhandler;
	}

	if ( (vertices_moves = calloc(g->nvertices,
			                      sizeof(*vertices_moves))) == NULL)
	{
		err = 1;
		goto errhandler;
	}

	for (i = 0; i < g->nvertices; i++)
		vertices_moves[i] = -1;

	/* Iterations of FM heuristic */
	for (iter = 0; iter < 1; iter++) {

		fm_pqueue_clear(pqueues[0]);
		fm_pqueue_clear(pqueues[1]);

		/* Permute boundary vertices and insert them in queues. */
	    for (i = 0; i < g->nvertices; i++)
	    	perm[i] = i;
	    for (i = 0; i < g->nvertices; i++) {
	    	j = g->nvertices * ((double)rand() / (double)(RAND_MAX + 1.0));
	    	tmp = perm[i];
	    	perm[i] = perm[j];
	    	perm[j] = tmp;
	    }

		for (i = 0; i < g->nvertices; i++) {
			j = perm[i];
			if (extcosts[j] > 0 || g->adjindexes[j] == g->adjindexes[j + 1]) {
				fm_pqueue_insert(pqueues[bisection[j]], j,
						         extcosts[j] - intcosts[j]);
			}
		}

		/* Move vertices from one partition to another */
		for (moveno = 0; moveno < nmoves; moveno++) {

			dstpart = moveto;
			srcpart = (dstpart + 1) % 2;

			/* Find in partition srcpart vertex with max gain */
			if ( (v = fm_pqueue_get_maxgain_vertex(pqueues[srcpart])) == -1) {
				break;
			}

			/* Move vertex v to partition dstpart */
			bisection[v] = dstpart;
			moves[moveno] = v;
			vertices_moves[v] = moveno;

			/* Update gain of moved vertex */
			gain = extcosts[v];
			extcosts[v] = intcosts[v];
			intcosts[v] = gain;

			/* Update gains of adj vertices */
			for (i = g->adjindexes[v]; i < g->adjindexes[v + 1]; i++) {
				j = g->adjv[i];
				gain_old = extcosts[j] - intcosts[j];
				extcost_old = extcosts[j];
				intcost_old = intcosts[j];

				/* Update gain of vertex j */
				tmp = (dstpart == bisection[j]) ? g->edges[i] : -g->edges[i];
				intcosts[j] += tmp;
				extcosts[j] -= tmp;

				if (extcost_old > 0) {
					/* Vertex j was a boundary vertex */
					if (extcosts[j] == 0) {
						if (vertices_moves[j] == -1) {
							fm_pqueue_delete(pqueues[bisection[j]], j, gain_old);
						}
					} else {
						if (vertices_moves[j] == -1) {
							fm_pqueue_update(pqueues[bisection[j]], j,
									         gain_old,
									         extcosts[j] - intcosts[j]);
						}
					}
				} else {
					if (extcosts[j] > 0) {
						/* Vertex j is a boundary vertex now */
						if (vertices_moves[j] == -1) {
							fm_pqueue_insert(pqueues[bisection[j]], j,
									         extcosts[j] - intcosts[j]);
						}
					}
				}
			}

		} /* for moves */

		/* Undo all moves to best move */
		for (i = 0; i < nmoves; i++)
			vertices_moves[moves[i]] = -1;

	} /* for FM iterations */

errhandler:
	free(extcosts);
	free(intcosts);
	free(moves);
	free(vertices_moves);
	free(perm);

	fm_pqueue_free(pqueues[0]);
	fm_pqueue_free(pqueues[1]);

	return err;
}

/* fm_pqueue_create: Create priority queue. */
fm_pqueue_t *fm_pqueue_create(csrgraph_t *g, gain_t gainmax)
{
	fm_pqueue_t *q = NULL;

	if ( (q = malloc(sizeof(*q))) == NULL) {
		return NULL;
	}
	q->hashtab = NULL;
	q->heap = NULL;
	if (g->nvertices < FM_HASHTABLE_VERTICES_LIMIT
		|| gainmax > FM_HASHTABLE_GAIN_LIMIT)
	{
		q->type = FM_PQUEUE_HEAP;
		if ( (q->heap = fm_heap_create(g)) == NULL) {
			free(q);
			return NULL;
		}
	} else {
		q->type = FM_PQUEUE_HASHTABLE;
		if ( (q->hashtab = fm_hashtab_create(g, (int)gainmax)) == NULL) {
			free(q);
			return NULL;
		}
	}
	return q;
}

/* fm_pqueue_free: */
void fm_pqueue_free(fm_pqueue_t *q)
{
	if (q->type == FM_PQUEUE_HEAP)
		fm_heap_free(q->heap);
	else
		fm_hashtab_free(q->hashtab);
	free(q);
}

/* fm_pqueue_clear: */
void fm_pqueue_clear(fm_pqueue_t *q)
{
	if (q->type == FM_PQUEUE_HEAP)
		fm_heap_clear(q->heap);
	else
		fm_hashtab_clear(q->hashtab);
}

/* fm_pqueue_insert: */
void fm_pqueue_insert(fm_pqueue_t *q, int v, gain_t gain)
{
	if (q->type == FM_PQUEUE_HEAP)
		fm_heap_insert(q->heap, v, gain);
	else
		fm_hashtab_insert(q->hashtab, v, gain);
}

/* fm_pqueue_get_maxgain_vertex: */
int fm_pqueue_get_maxgain_vertex(fm_pqueue_t *q)
{
	if (q->type == FM_PQUEUE_HEAP)
		return fm_heap_get_maxgain_vertex(q->heap);
	else
		return fm_hashtab_get_maxgain_vertex(q->hashtab);
}

/* fm_pqueue_delete: */
void fm_pqueue_delete(fm_pqueue_t *q, int v, gain_t gain)
{
	if (q->type == FM_PQUEUE_HEAP)
		fm_heap_delete(q->heap, v, gain);
	else
		fm_hashtab_delete(q->hashtab, v, gain);
}

/* fm_pqueue_update: */
void fm_pqueue_update(fm_pqueue_t *q, int v, gain_t gainold,
					  gain_t gainnew)
{
	if (q->type == FM_PQUEUE_HEAP)
		fm_heap_update(q->heap, v, gainold, gainnew);
	else
		fm_hashtab_update(q->hashtab, v, gainold, gainnew);
}

/* fm_heap_create: */
fm_heap_t *fm_heap_create(csrgraph_t *g)
{
	int i;
	fm_heap_t *h;

	if ( (h = malloc(sizeof(*h))) == NULL)
		return NULL;
	h->nvertices = 0;
	h->nvertices_max = g->nvertices;
	h->heap = NULL;
	h->pointer = NULL;

	h->heap = malloc(sizeof(*h->heap) * g->nvertices);
	h->pointer = malloc(sizeof(*h->pointer) * g->nvertices);
	if (h->heap == NULL || h->pointer == NULL) {
		free(h->heap);
		free(h->pointer);
		free(h);
		return NULL;
	}

	for (i = 0; i < h->nvertices_max; i++)
		h->pointer[i] = -1;

	return h;
}

/* fm_heap_free: */
void fm_heap_free(fm_heap_t *h)
{
	free(h->heap);
	free(h->pointer);
	free(h);
}

/* fm_heap_clear: */
void fm_heap_clear(fm_heap_t *h)
{
	int i;

	h->nvertices = 0;
	for (i = 0; i < h->nvertices_max; i++)
		h->pointer[i] = -1;
}

/* fm_heap_insert: */
void fm_heap_insert(fm_heap_t *h, int v, gain_t gain)
{
	int i, j;

	i = h->nvertices;
	h->nvertices++;

	while (i > 0) {
		j = (i - 1) / 2;
		if (h->heap[j].key < gain) {
			h->heap[i] = h->heap[j];
			h->pointer[h->heap[i].value] = i;
	        i = j;
		} else {
	        break;
		}
	}
	h->heap[i].key = gain;
	h->heap[i].value = v;
	h->pointer[v] = i;
}

/* fm_heap_get_maxgain_vertex: */
int fm_heap_get_maxgain_vertex(fm_heap_t *h)
{
	int i, j, v, vertex, gain;

	if (h->nvertices == 0)
		return -1;

	h->nvertices--;

    vertex = h->heap[0].value;
    h->pointer[vertex] = -1;

    if ( (i = h->nvertices) > 0) {
    	gain = h->heap[i].key;
    	v = h->heap[i].value;
    	i = 0;

    	while ((j = 2 * i + 1) < h->nvertices) {
    		if (h->heap[j].key > gain) {
    			if (j + 1 < h->nvertices && h->heap[j + 1].key > h->heap[j].key)
    				j = j + 1;
    			h->heap[i] = h->heap[j];
    			h->pointer[h->heap[i].value] = i;
    			i = j;
    		} else if (j + 1 < h->nvertices && h->heap[j + 1].key > gain) {
    			j = j + 1;
    			h->heap[i] = h->heap[j];
    			h->pointer[h->heap[i].value] = i;
    			i = j;
    		} else {
    			break;
    		}
    	}
    	h->heap[i].key = gain;
    	h->heap[i].value = v;
    	h->pointer[v] = i;
    }

    return vertex;
}

/* fm_heap_delete: */
void fm_heap_delete(fm_heap_t *h, int v, gain_t gain)
{
	int i, j;
	int gainnew, gainold;

	i = h->pointer[v];
	h->pointer[v] = -1;

	h->nvertices--;
	if (h->nvertices > 0 && h->heap[h->nvertices].value != v) {
		v = h->heap[h->nvertices].value;
		gainnew = h->heap[h->nvertices].key;
	    gainold = h->heap[i].key;

	    if (gainold < gainnew) {
	        while (i > 0) {
	    		j = (i - 1) >> 1;
	    		if (h->heap[j].key < gainnew) {
	    			h->heap[i] = h->heap[j];
					h->pointer[h->heap[i].value] = i;
					i = j;
	    		} else {
	    			break;
	    		}
	    	}

	    } else {

	        while ( (j = 2 * i + 1) < h->nvertices) {
	        	if (h->heap[j].key > gainnew) {
	        		if (j + 1 < h->nvertices
	        		    && h->heap[j + 1].key > h->heap[j].key)
	        		{
	        			j++;
	        		}
	        		h->heap[i] = h->heap[j];
	        		h->pointer[h->heap[i].value] = i;
	        		i = j;
	        	} else if (j +1 < h->nvertices
	        			   && h->heap[j + 1].key > gainnew)
	        	{
	        		j++;
	        		h->heap[i] = h->heap[j];
	        		h->pointer[h->heap[i].value] = i;
	        		i = j;
	        	} else {
	        		break;
	        	}
	        }
	    }

	    h->heap[i].key = gainnew;
	    h->heap[i].value = v;
	    h->pointer[v] = i;
	}
}

/* fm_heap_update: */
void fm_heap_update(fm_heap_t *h, int v, gain_t gainold, gain_t gainnew)
{
	int i, j;

	i = h->pointer[v];
	if (gainold < gainnew) {
		while (i > 0) {
			j = (i - 1) >> 1;
			if (h->heap[j].key < gainnew) {
				h->heap[i] = h->heap[j];
				h->pointer[h->heap[i].value] = i;
				i = j;
	        } else {
	        	break;
	        }
		}
	} else {
		while ( (j = 2 * i + 1) < h->nvertices) {
			if (h->heap[j].key > gainnew) {
				if (j + 1 < h->nvertices && h->heap[j + 1].key > h->heap[j].key)
					j++;
				h->heap[i] = h->heap[j];
				h->pointer[h->heap[i].value] = i;
	            i = j;
			} else if (j + 1 < h->nvertices && h->heap[j + 1].key > gainnew) {
				j = j + 1;
				h->heap[i] = h->heap[j];
				h->pointer[h->heap[i].value] = i;
				i = j;
			} else {
				break;
			}
		}
	}
	h->heap[i].key = gainnew;
	h->heap[i].value = v;
	h->pointer[v] = i;
}

/* fm_hashtab_create: */
fm_hashtab_t *fm_hashtab_create(csrgraph_t *g, gain_t gainmax)
{
	int i;
	fm_hashtab_t *b;

	if ( (b = malloc(sizeof(*b))) == NULL) {
		return NULL;
	}

	/* Computer max gain */
	b->gain_max = gainmax;

	b->nvertices = 0;
	b->size = 1 + 2 * b->gain_max;

    b->hashtab = NULL;
	b->vertices = NULL;

	b->hashtab = malloc(sizeof(fm_hashtab_vertex_t *) * b->size);
	b->vertices = malloc(sizeof(fm_hashtab_vertex_t) * g->nvertices);
	if (b->hashtab == NULL || b->vertices == NULL) {
		free(b->hashtab);
		free(b->vertices);
		free(b);
		return NULL;
	}

	for (i = 0; i < g->nvertices; i++)
		b->vertices[i].v = i;
	for (i = 0; i < b->size; i++)
		b->hashtab[i] = NULL;
	b->gain_max_ptr = -b->gain_max;
	b->hashtab += b->gain_max;
	return b;
}

/* fm_hashtab_insert: */
void fm_hashtab_insert(fm_hashtab_t *b, int v, gain_t gain)
{
	fm_hashtab_vertex_t *newv;

	b->nvertices++;
	newv = &b->vertices[v];
	newv->next = b->hashtab[gain];
	b->hashtab[gain] = newv;

	/* Update pointer to max gain */
	if (b->gain_max_ptr < gain)
		b->gain_max_ptr = gain;
}

/* fm_hashtab_clear: */
void fm_hashtab_clear(fm_hashtab_t *b)
{
	int i;

	b->nvertices = 0;
	b->hashtab -= b->gain_max;
	for (i = 0; i < b->size; i++)
		b->hashtab[i] = NULL;
	b->hashtab += b->gain_max;
	b->gain_max_ptr = -b->gain_max;
}

/* fm_hashtab_get_maxgain_vertex: */
int fm_hashtab_get_maxgain_vertex(fm_hashtab_t *b)
{
	fm_hashtab_vertex_t *vptr;

	if (b->nvertices == 0)
		return -1;

	b->nvertices--;
	vptr = b->hashtab[b->gain_max_ptr];
	b->hashtab[b->gain_max_ptr] = vptr->next;

	if (vptr->next == NULL) {
		/* Update gain ptr */
		if (b->nvertices == 0)
			b->gain_max_ptr = -b->gain_max;
		else
			for ( ; b->hashtab[b->gain_max_ptr] == NULL; b->gain_max_ptr--);
	}
	return vptr->v;
}

/* fm_hashtab_delete: */
void fm_hashtab_delete(fm_hashtab_t *b, int v, gain_t gain)
{
	fm_hashtab_vertex_t *vptr, *pv, *prev;

	vptr = b->vertices + v;
	b->nvertices--;

	prev = NULL;
	for (pv = b->hashtab[gain]; pv != NULL; pv = pv->next) {
		if (pv->v == v)
			break;
		prev = pv;
	}

	if (prev != NULL)
		prev->next = vptr->next;
	else
		b->hashtab[gain] = vptr->next;

	if (b->hashtab[gain] == NULL || gain == b->gain_max_ptr) {
		/* Update gain ptr */
		if (b->nvertices != 0)
			for ( ; b->hashtab[b->gain_max_ptr] == NULL; b->gain_max_ptr--);
		else
			b->gain_max_ptr = -b->gain_max;
	}
}

/* fm_hashtab_update: */
void fm_hashtab_update(fm_hashtab_t *b, int v, gain_t gainold, gain_t gainnew)
{
	if (gainold != gainnew) {
		fm_hashtab_delete(b, v, gainold);
		fm_hashtab_insert(b, v, gainnew);
	}
}

/* fm_hashtab_free: */
void fm_hashtab_free(fm_hashtab_t *b)
{
	if (b != NULL) {
		b->hashtab -= b->gain_max;
		free(b->hashtab);
		free(b->vertices);
		free(b);
	}
}

