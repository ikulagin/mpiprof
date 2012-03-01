/*
 * example.c: 
 *
 * (C) 2011 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "gpart.h"

int main(int argc, char **argv)
{
	int *pweights = NULL, *part = NULL;
	int i, j, nparts;
	
	if ( (part = malloc(sizeof(*part) * comm->nprocesses)) == NULL) {
		err = TOPOMPI_RC_FAILURE;
		goto errhandler;
	}
	if ( (pweights = malloc(sizeof(*pweights) * comm->nnodes)) == NULL) {
		err = TOPOMPI_RC_FAILURE;
		goto errhandler;
	}
	for (i = 0; i < comm->nnodes; i++)
		pweights[i] = comm->nodes[i].nprocesses;
	nparts = comm->nnodes;

	/* Generate Bruck's algorithm task graph */
	if ( (graph = csrgraph_generate_bruck(comm->nprocesses)) == NULL) {
		err = TOPOMPI_RC_FAILURE;
		goto errhandler;
	}

	/* Partition graph */
	if (gpart_partition_recursive_regular(graph, pweights, nparts, part) > 0) {
		err = TOPOMPI_RC_FAILURE;
		goto errhandler;
	}
	csrgraph_free(graph);
	graph = NULL;
	free(pweights);
	pweights = NULL;

	return EXIT_SUCCESS;
}
