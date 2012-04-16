/*
 * control_monitoring.h
 *
 *  Created on: 10.03.2012
 *      Author: ikulagin
 */

#ifndef CONTROL_MONITORING_H_
#define CONTROL_MONITORING_H_

#include <stdio.h>
#include <mpi.h>

typedef struct control_mon_event_s {
    int (*mpiprof_allreduce)(void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*mpiprof_init)(int *argc, char ***argv);
    int (*mpiprof_comm_size)(MPI_Comm comm, int *size);
    int (*mpiprof_comm_rank)(MPI_Comm comm, int *rank);
    int (*mpiprof_comm_split)(MPI_Comm comm, int color, int key,
                              MPI_Comm *newcomm);
    int (*mpiprof_comm_dup)(MPI_Comm comm, MPI_Comm *newcomm);
    int (*mpiprof_abort)(MPI_Comm comm, int errorcode);
    int (*mpiprof_bcast)(void *buffer, int count, MPI_Datatype datatype,
                         int root, MPI_Comm comm);
    int (*mpiprof_barrier)(MPI_Comm comm);
    int (*mpiprof_isend)(void *buf, int count, MPI_Datatype datatype,
                         int dest,int tag, MPI_Comm comm, MPI_Request *request);
    int (*mpiprof_irecv)(void *buf, int count, MPI_Datatype datatype,
                         int source, int tag, MPI_Comm comm,
                         MPI_Request *request);
    int (*mpiprof_reduce)(void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm);

    int (*mpiprof_wait)(MPI_Request *request, MPI_Status *status);
    int (*mpiprof_waitall)(int count, MPI_Request *array_of_requests,
                           MPI_Status *array_of_statuses);
    int (*mpiprof_finalize)();

} control_mon_event_t;

enum {
    MPIPROFUSE,
    MPIPROFGEN,
};

control_mon_event_t *mpiprof_init(int mode);

#endif /* CONTROL_MONITORING_H_ */
