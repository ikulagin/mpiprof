/*
 *  communication.h
 *  BT_Wrappers
 *
 *  Created by Ivan Kulagin on 09.02.11.
 *
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <mpi.h>

typedef struct comm comm_t;
typedef struct commtable commtable_t;

commtable_t *commtable_create(int n);

void commtable_add_msgsize(int partner, uint64_t msgsize, commtable_t *table);
void commtable_add_datatype(int partner, int count, MPI_Datatype datatype,
                            commtable_t *table);
void commtable_print(int rank, commtable_t *table);
void commtable_free(commtable_t *table);
int commtable_fill_vectors(commtable_t *table, int *vertices, uint64_t *waight,
                           int *nedges);

#endif /*COMMUNICATION_H*/
