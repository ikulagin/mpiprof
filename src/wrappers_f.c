/*
 * wrappers.c
 *
 *  Created on: 10.03.2012
 *      Author: ikulagin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#include "control_monitoring.h"
#include "log.h"

control_mon_event_t *general_obj; //??????? =)

void mpi_init_(MPI_Fint *ierr)
{

    int rc;

    if ((rc = PMPI_Init(NULL, NULL)) == MPI_SUCCESS) {
        if (!strcmp(getenv("MPIPROF_MODE"), "mpiprofgen")) {
            general_obj = mpiprof_init(MPIPROFGEN);
            apptrace(TRACE_INFO_BIT, "mpiprofgen mode\n");
        } else if (!strcmp(getenv("MPIPROF_MODE"), "mpiprofuse")) {
            general_obj = mpiprof_init(MPIPROFUSE);
        } else {
            apptrace(TRACE_ERR_BIT, "unknown mode\n");
            PMPI_Abort(MPI_COMM_WORLD, 0);
        }
    }
    if (general_obj->mpiprof_init(NULL, NULL) != MPI_SUCCESS) {
        PMPI_Abort(MPI_COMM_WORLD, 0);
    }

    *ierr = (MPI_Fint)rc;
}

/* ===================================================== */
void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_comm_size(MPI_Comm_f2c(*comm),
                                                     (int *)size);
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_comm_rank(MPI_Comm_f2c(*comm),
                                                     (int *)rank);
}

void mpi_abort_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_abort(MPI_Comm_f2c(*comm),
                                                 (int)*errorcode);
}

void mpi_bcast_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root,
                MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_bcast(buf, (int)*count,
                                                 MPI_Type_f2c(*datatype),
                                                 (int)*root,
                                                 MPI_Comm_f2c(*comm));
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_barrier(MPI_Comm_f2c(*comm));
}

void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key,
                     MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;

    *ierr = (MPI_Fint)general_obj->mpiprof_comm_split(MPI_Comm_f2c(*comm), (int)*color,
                                                      (int)*key, &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);
}
void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;

    *ierr = (MPI_Fint)general_obj->mpiprof_comm_dup(MPI_Comm_f2c(*comm),
                                                    &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);
}
void mpi_reduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                 MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root,
                 MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_reduce(sendbuf, recvbuf, (int)*count,
                                                  MPI_Type_f2c(*datatype),
                                                  MPI_Op_f2c(*op), (int)*root,
                                                  MPI_Comm_f2c(*comm));
}

void mpi_allreduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                    MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
                    MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_allreduce(sendbuf, recvbuf,
                                                     (int)*count,
                                                     MPI_Type_f2c(*datatype),
                                                     MPI_Op_f2c(*op),
                                                     MPI_Comm_f2c(*comm));
}

/* ===================================================== */

void mpi_isend_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest,
                MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
                MPI_Fint *ierr)
{
    MPI_Request tmp_request;

    *ierr = (MPI_Fint)general_obj->mpiprof_isend(buf, (int)*count,
                                                 MPI_Type_f2c(*datatype),
                                                 (int)*dest, (int)*tag,
                                                 MPI_Comm_f2c(*comm),
                                                 &tmp_request);
    *request = MPI_Request_c2f(tmp_request);
}

void mpi_irecv_(void *buf, MPI_Fint *count, MPI_Fint *datatype,
                MPI_Fint *source,MPI_Fint *tag, MPI_Fint *comm,
                MPI_Fint *request,MPI_Fint *ierr)
{
    MPI_Request tmp_request;

    *ierr = (MPI_Fint)general_obj->mpiprof_irecv(buf, (int)*count,
                                                 MPI_Type_f2c(*datatype),
                                                 (int)*source, (int)*tag,
                                                 MPI_Comm_f2c(*comm),
                                                 &tmp_request);
    *request = MPI_Request_c2f(tmp_request);
}

void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Request tmp_request;
    MPI_Status tmp_status;
    int rc;

    MPI_Status_f2c(status, &tmp_status);
    tmp_request = MPI_Request_f2c(*request);


    *ierr = (MPI_Fint)general_obj->mpiprof_wait(&tmp_request, &tmp_status);

    *request = MPI_Request_c2f(tmp_request);
    MPI_Status_c2f(&tmp_status, status);
}

void mpi_waitall_(MPI_Fint *count, MPI_Fint array_of_request[],
                  MPI_Fint array_of_status[], MPI_Fint *ierr)
{
    MPI_Request *tmp_request;
    MPI_Status *tmp_status;
    int i;

    tmp_status = malloc(sizeof(MPI_Status) * ((int)*count));
    tmp_request = malloc(sizeof(MPI_Request) * ((int)*count));

    for (i = 0; i < (int)*count; i++) {
        MPI_Status_f2c(&array_of_status[i], &tmp_status[i]);
        tmp_request[i] = MPI_Request_f2c(array_of_request[i]);
    }

    *ierr = (MPI_Fint)general_obj->mpiprof_waitall((int)*count, tmp_request,
                                                   tmp_status);

    for (i = 0; i < (int)*count; i++) {
        array_of_request[i] = MPI_Request_c2f(tmp_request[i]);
        PMPI_Status_c2f(&tmp_status[i], &array_of_status[i]);
    }

    free(tmp_status);
    free(tmp_request);
}

void mpi_finalize_(MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)general_obj->mpiprof_finalize();
}
