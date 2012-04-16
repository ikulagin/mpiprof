/*
 * control_monitoring.c
 *
 *  Created on: 26.03.2012
 *      Author: ikulagin
 */

#include <stdlib.h>

#include "control_monitoring.h"
#include "log.h"

int mpiprofgen_init(int *argc, char ***argv);
int mpiprofgen_isend(void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request);
int mpiprofgen_irecv(void *buf, int count, MPI_Datatype datatype,
        int source, int tag, MPI_Comm comm, MPI_Request *request);
int mpiprofgen_wait(MPI_Request *request, MPI_Status *status);
int mpiprofgen_waitall(int count, MPI_Request *array_of_requests,
                       MPI_Status *array_of_statuses);
int mpiprofgen_finalize();

control_mon_event_t *mpiprof_init(int mode)
{
    control_mon_event_t *tmp;

    tmp = malloc(sizeof(control_mon_event_t));
    if (tmp == NULL) {
        return NULL;
    }

    if (mode == MPIPROFGEN) {
        tmp->mpiprof_abort = PMPI_Abort;
        tmp->mpiprof_allreduce = PMPI_Allreduce;
        tmp->mpiprof_barrier = PMPI_Barrier;
        tmp->mpiprof_bcast = PMPI_Bcast;
        tmp->mpiprof_comm_dup = PMPI_Comm_dup;
        tmp->mpiprof_comm_rank = PMPI_Comm_rank;
        tmp->mpiprof_comm_size = PMPI_Comm_size;
        tmp->mpiprof_comm_split = PMPI_Comm_split;
        tmp->mpiprof_reduce = PMPI_Reduce;
        tmp->mpiprof_init = mpiprofgen_init;
        tmp->mpiprof_isend = mpiprofgen_isend;
        tmp->mpiprof_irecv = mpiprofgen_irecv;
        tmp->mpiprof_wait = mpiprofgen_wait;
        tmp->mpiprof_waitall = mpiprofgen_waitall;
        tmp->mpiprof_finalize = mpiprofgen_finalize;
    }

    if (mode == MPIPROFUSE) {
        apptrace(TRACE_DBG_BIT, "select MPIPROFUSE\n");
        return NULL;
    }

    return tmp;
}

void mpiprof_finalize(control_mon_event_t *mpiprof)
{
    apptrace(TRACE_DBG_BIT, "mpiprof_finalize\n");
    free(mpiprof);
    mpiprof = NULL;
}

