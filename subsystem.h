/*
 *  nodes.h
 *  Wrappers
 *
 *  Created by Ivan Kulagin on 09.02.11.
 */
#ifndef NODES_H
#define NODES_H

#include <stdio.h>
#include <stdlib.h>

typedef struct nodes nodes_t;


nodes_t *subsystem_init(int *mapp, int commsize, int nparts,
                        int *pweights, int *subset_nodes);
void subsystem_free(nodes_t *subsystem, int nparts);
int subsystem_getproc(nodes_t *subsystem, int subset);
#endif /* NODES_H */
