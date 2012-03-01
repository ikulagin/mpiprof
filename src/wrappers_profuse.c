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

#include "profusemode.h"

void mpi_init_(MPI_Fint *ierr)
{
    int rc;
    /* @mk: Зачем здесь пустые строки ?*/
    if ((rc = PMPI_Init(NULL, NULL)) == MPI_SUCCESS) {
        profuse_init();
    }
    
    *ierr = (MPI_Fint) rc;
}

/* ===================================================== */
void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_comm_size(MPI_Comm_f2c(*comm), (int *)size);
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_comm_rank(MPI_Comm_f2c(*comm), (int *)rank);
}

void mpi_abort_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_abort(MPI_Comm_f2c(*comm), (int)*errorcode);
}

void mpi_bcast_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, 
                MPI_Fint *comm, MPI_Fint *ierr)
{
    printf("bc\n");
    *ierr = (MPI_Fint)profuse_bcast(buf, (int)*count, MPI_Type_f2c(*datatype),
                                    (int)*root, MPI_Comm_f2c(*comm));
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_barrier(MPI_Comm_f2c(*comm));
}

void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key,
                     MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;
    
    *ierr = (MPI_Fint)profuse_comm_split(MPI_Comm_f2c(*comm), (int)*color,
                                         (int)*key, &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);
}
void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm_, MPI_Fint *ierr)
{
    MPI_Comm tmpcomm;
    
    *ierr = (MPI_Fint)profuse_comm_dup(MPI_Comm_f2c(*comm), &tmpcomm);
    *newcomm_ = MPI_Comm_c2f(tmpcomm);
}
void mpi_reduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                 MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, 
                 MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_reduce(sendbuf, recvbuf, (int)*count,
                                     MPI_Type_f2c(*datatype), MPI_Op_f2c(*op),
                                     (int)*root, MPI_Comm_f2c(*comm));
}

void mpi_allreduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                    MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
                    MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_allreduce(sendbuf, recvbuf, (int)*count,
                                     MPI_Type_f2c(*datatype), MPI_Op_f2c(*op),
                                        MPI_Comm_f2c(*comm));
}

/* ===================================================== */

void mpi_isend_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest,
                MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
                MPI_Fint *ierr)
{
    MPI_Request tmp_request;

    *ierr = (MPI_Fint)profuse_isend(buf, (int)*count, MPI_Type_f2c(*datatype),
                                (int)*dest, (int)*tag, MPI_Comm_f2c(*comm),
                                &tmp_request);
    *request = MPI_Request_c2f(tmp_request);
}

void mpi_irecv_(void *buf, MPI_Fint *count, MPI_Fint *datatype,
                MPI_Fint *source,MPI_Fint *tag, MPI_Fint *comm,
                MPI_Fint *request,MPI_Fint *ierr)
{
    MPI_Request tmp_request;

    *ierr = (MPI_Fint)profuse_irecv(buf, (int)*count, MPI_Type_f2c(*datatype),
                                    (int)*source, (int)*tag,
                                    MPI_Comm_f2c(*comm), &tmp_request);
    *request = MPI_Request_c2f(tmp_request);
}

void mpi_finalize_(MPI_Fint *ierr)
{
    *ierr = (MPI_Fint)profuse_finalize();
}
