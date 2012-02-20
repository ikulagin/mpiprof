/*
 * example_part.c: 
 *
 * (C) 2011 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "gpart.h"

int main(int argc, char **argv)
{
    csrgraph_t *graph;
    int *pweights, *part;
	int i, nparts;
    
    if (argc < 3) {
        fprintf(stderr, "Usage: example_part <graphfile> <nparts>\n");
        exit(EXIT_FAILURE);
    }
    nparts = atoi(argv[2]);    

    if ( (graph = csrgraph_load(argv[1])) == NULL) {
        fprintf(stderr, "Can not load graph file\n");
        exit(EXIT_FAILURE);
    }
	if ( (part = malloc(sizeof(*part) * graph->nvertices)) == NULL) {
        fprintf(stderr, "No enough memory\n");
        exit(EXIT_FAILURE);
	}
	if ( (pweights = malloc(sizeof(*pweights) * nparts)) == NULL) {
        fprintf(stderr, "No enough memory\n");
        exit(EXIT_FAILURE);
	}

    /* pweights[i] - number of required vertices in partition i */
	for (i = 0; i < nparts; i++)
		pweights[i] = graph->nvertices / nparts;

	/* Partition graph */
	if (gpart_partition_recursive(graph, pweights, nparts, part) > 0) {
        fprintf(stderr, "Error partitioning graph\n");
        exit(EXIT_FAILURE);
	}
    
    printf("# vertex -> subset (partition)\n");
    for (i = 0; i < graph->nvertices; i++) {
        printf("%d %d\n", i + 1, part[i] + 1);
    }
    
	csrgraph_free(graph);
	free(pweights);
	free(part);
	return EXIT_SUCCESS;
}
