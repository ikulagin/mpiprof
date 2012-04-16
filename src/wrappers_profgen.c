/*
 * wrappers.c: 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#include <mpi.h>

#include "profgenmode.h"


int mpiprofgen_init(int *argc, char ***argv)
{
    profgenmode_init();
    return MPI_SUCCESS;
}

int mpiprofgen_isend(void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request)
{
    int rc;


    rc = MPI_Isend(buf, count, datatype, dest, tag, comm, request);
    if (rc == MPI_SUCCESS) {
        profgenmode_reg_pt2pt(count, dest, datatype, *request);
    }

    return rc;
}

int mpiprofgen_irecv(void *buf, int count, MPI_Datatype datatype,
        int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int rc;

    rc = MPI_Irecv(buf, count, datatype, source, tag, comm, request);
    if (rc == MPI_SUCCESS) {
        profgenmode_reg_pt2pt(count, source, datatype, *request);
    }

    return rc;
}

int mpiprofgen_wait(MPI_Request *request, MPI_Status *status)
{
    
    profgenmode_complete_pt2pt_one(*request);
    
    return PMPI_Wait(request, status);
}

int mpiprofgen_waitall(int count, MPI_Request *array_of_requests,
                       MPI_Status *array_of_statuses)
{
    profgenmode_complete_pt2pt_all(array_of_requests, count);
    
    return PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

int mpiprofgen_finalize()
{
    profgenmode_finalize();
    return MPI_Finalize();
}
