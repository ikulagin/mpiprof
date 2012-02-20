/*
 *  reqlist.h
 *  BT_Wrappers
 *
 *  Created by Ivan Kulagin on 09.02.11.
 */
#ifndef REQLIST_H
#define REQLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include <mpi.h>

#include "communication.h"

enum {
    REQ_CONFIRMED = 1,
    REQ_NOT_CONFIRMED = -1,
    REQ_SEND = 2,
    REQ_RECV = 3,
};

typedef struct request request_t;

typedef struct reqlist reqlist_t;

int reqlist_add(int count, int partner, 
                MPI_Datatype datatype, MPI_Request req, reqlist_t *list);
reqlist_t *reqlist_create();

request_t *reqlist_lookup(MPI_Request req, reqlist_t *list);

void reqlist_elem_complate(request_t **elem, commtable_t *table); /*подтверждение*/

void reqlist_fill_commtable(reqlist_t *list, commtable_t *table);

void reqlist_print(reqlist_t *list);

#endif /*REQLIST_H*/
