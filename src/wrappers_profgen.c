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


void mpi_init_(MPI_Fint *ierr)
{
    int rc;
    if ((rc = PMPI_Init(NULL, NULL)) == MPI_SUCCESS) {
        profgenmode_init();
    }
    *ierr = (MPI_Fint)rc;
}

/*void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    int rc;
    printf("COMM_SIZE\n");
    rc = MPI_Comm_size( MPI_Comm_f2c(*comm), (int *)size );
    *ierr = (MPI_Fint)rc;
    
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    printf("COMM_RANK\n");
    
    *ierr = (MPI_Fint)MPI_Comm_rank( MPI_Comm_f2c(*comm), (int *)rank);
}

void mpi_abort_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    printf("ABORT\n");
 
        *ierr = (MPI_Fint)MPI_Abort( MPI_Comm_f2c(*comm), (int)*errorcode );
}

void mpi_bcast_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, 
                MPI_Fint *comm, MPI_Fint *ierr)
{
    int rc;
    printf("BCAST %d ---> ...\n", *root);
    *ierr = (MPI_Fint)MPI_Bcast(buf, (int)*count, 
                                MPI_Type_f2c(*datatype), (int)*root,
                                MPI_Comm_f2c(*comm));
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
    printf("BARRIER\n");
    *ierr = (MPI_Fint)MPI_Barrier(MPI_Comm_f2c(*comm));
}

void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key,
                     MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;
    
    printf("Comm_split\n");
    *ierr = (MPI_Fint)MPI_Comm_split(MPI_Comm_f2c(*comm), (int)*color,
                                     (int)*key, &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);

}
void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;
    
    printf("Comm_dup\n");
    *ierr = (MPI_Fint)MPI_Comm_dup(MPI_Comm_f2c(*comm), &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);
}
void mpi_reduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                 MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, 
                 MPI_Fint *comm, MPI_Fint *ierr)
{
    printf("REDUCE\n");
    *ierr = (MPI_Fint)MPI_Reduce(sendbuf, recvbuf, (int)*count,
                                 MPI_Type_f2c(*datatype), MPI_Op_f2c(*op),
                                 (int)*root, MPI_Comm_f2c(*comm));
}
*/
/* ===================================================== */

void mpi_isend_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest,
                MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
                MPI_Fint *ierr)
{
    int rc;
    MPI_Request tmp_request;

    rc = MPI_Isend(buf, (int)*count, MPI_Type_f2c(*datatype), (int)*dest,
                   (int)*tag, MPI_Comm_f2c(*comm), &tmp_request);
    if (rc == MPI_SUCCESS) {
        profgenmode_reg_pt2pt((int)*count, (int)*dest, MPI_Type_f2c(*datatype),
                              tmp_request);
    }
    *request = MPI_Request_c2f(tmp_request);
    *ierr = (MPI_Fint)rc;
}

void mpi_irecv_(void *buf, MPI_Fint *count, MPI_Fint *datatype,
                MPI_Fint *source,MPI_Fint *tag, MPI_Fint *comm,
                MPI_Fint *request,MPI_Fint *ierr)
{
    int rc;
    MPI_Request tmp_request;

    rc = MPI_Irecv(buf, (int)*count, MPI_Type_f2c(*datatype), (int)*source,
                   (int)*tag, MPI_Comm_f2c(*comm), &tmp_request);
    if (rc == MPI_SUCCESS) {
        profgenmode_reg_pt2pt((int)*count, (int)*source,
                              MPI_Type_f2c(*datatype), tmp_request);
    }
    *request = MPI_Request_c2f(tmp_request);
    *ierr = (MPI_Fint)rc;
}

void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Request tmp_request;
    MPI_Status tmp_status;
    int rc;

    MPI_Status_f2c(status, &tmp_status);
    tmp_request = MPI_Request_f2c(*request);
    
    profgenmode_complete_pt2pt_one(tmp_request);
    
    rc = MPI_Wait(&tmp_request, &tmp_status);
    
    *request = MPI_Request_c2f(tmp_request);
    MPI_Status_c2f(&tmp_status, status);
    
    *ierr = (MPI_Fint)rc;
}

void mpi_waitall_(MPI_Fint *count, MPI_Fint array_of_request[],
                  MPI_Fint array_of_status[], MPI_Fint *ierr)
{
    MPI_Request *tmp_request;
    MPI_Status *tmp_status;
    int rc, i;
    
    tmp_status = malloc(sizeof(MPI_Status) * ((int)*count));
    tmp_request = malloc(sizeof(MPI_Request) * ((int)*count));
    
    for (i = 0; i < (int)*count; i++) {
        MPI_Status_f2c(&array_of_status[i], &tmp_status[i]);
        tmp_request[i] = MPI_Request_f2c(array_of_request[i]);
    }
    
    profgenmode_complete_pt2pt_all(tmp_request, (int)*count);
    
    rc = PMPI_Waitall((int)*count, tmp_request, tmp_status);
    
    for (i = 0; i < (int)*count; i++) {
        array_of_request[i] = MPI_Request_c2f(tmp_request[i]);
        PMPI_Status_c2f(&tmp_status[i], &array_of_status[i]);
    }
    
    free(tmp_status);
    free(tmp_request);
    
    *ierr = (MPI_Fint)rc;
}

void mpi_finalize_(MPI_Fint *ierr)
{
    int rc;

    profgenmode_finalize();
    rc = MPI_Finalize();

    *ierr = (MPI_Fint)rc;
}
