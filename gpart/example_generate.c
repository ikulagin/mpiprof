/*
 * example_generate.c: 
 *
 * (C) 2011 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "gpart.h"

int main(int argc, char **argv)
{
    csrgraph_t *graph;

	/* Generate Bruck's algorithm task graph */
	if ( (graph = csrgraph_generate_bruck(64)) == NULL) {
		fprintf(stderr, "Can not create graph\n");
		exit(EXIT_FAILURE);
	}
    csrgraph_save(graph, "graph.csr");
    csrgraph_free(graph);
	return EXIT_SUCCESS;
}
