/*
 *  mapping.h
 *  BT_Wrappers
 *
 *  Created by Ivan Kulagin on 08.05.11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef MAPPING_H
#define MAPPING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#include <mpi.h>

#include "subsystem.h"
#include "gpart/gpart.h"
#include "algo.h"

enum {
    LENGTH_HOSTNAME = 50,
};

int *old_mapp, *new_mapp;
int *subset_nodes, *pweights;
int npart;

nodes_t *subsystem;

int getnodeid();
void primary_mapp(int rank, int commsize);
int mapping_initialize(int commsize);
int maping_allocate(int commsize, char *graph, int *ranks, char *mpipgo_algo);
void mapping_free();

#endif /*MAPPING_H*/
