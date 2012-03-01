/*
 * profusemode.h: Mode of implementation mappin.
 * Created by Ivan Kulagin <kadet901@gmail.com>
 */
#ifndef PROFUSE_MODE_H
#define PROFUSE_MODE_H
#include <sys/time.h>

#include <stdio.h>
#include <strings.h>

#include <mpi.h>

#include "mapping.h"

char *mpipgo_graph;
char *mpipgo_algo;
int profuse_commsize;
int profuse_rank;
double mapp_begin, mapp_end, test_begin, test_end;

/*
 * getnodeid: Function returns id of node.
 */

void profuse_init();
int profuse_comm_size(MPI_Comm comm, int *size);
int profuse_comm_rank(MPI_Comm comm, int *rank);
int profuse_abort(MPI_Comm comm, int errcode);
int profuse_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm_);
int profuse_comm_dup(MPI_Comm comm, MPI_Comm *newcomm_);
int profuse_reduce(void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int profuse_allreduce(void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int profuse_isend(void *buf, int count, MPI_Datatype datatype, int dest,
                  int tag, MPI_Comm comm, MPI_Request *request);
int profuse_irecv(void *buf, int count, MPI_Datatype datatype, int source,
                  int tag, MPI_Comm comm, MPI_Request *request);
int profuse_barrier(MPI_Comm comm);
int profuse_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                  MPI_Comm comm );
int profuse_finalize();
#endif /*PROFUSE_MODE_H*/
