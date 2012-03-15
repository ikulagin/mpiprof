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

#include <mpi.h>

void mpi_init_(MPI_Fint *ierr)
{
}

/* ===================================================== */
void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
}

void mpi_abort_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
}

void mpi_bcast_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root,
                MPI_Fint *comm, MPI_Fint *ierr)
{
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
}

void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key,
                     MPI_Fint *newcomm_, MPI_Fint *ierr)
{
}
void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm_, MPI_Fint *ierr)
{
}
void mpi_reduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                 MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root,
                 MPI_Fint *comm, MPI_Fint *ierr)
{
}

void mpi_allreduce_(void *sendbuf, void *recvbuf, MPI_Fint *count,
                    MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
                    MPI_Fint *ierr)
{
}

/* ===================================================== */

void mpi_isend_(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest,
                MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
                MPI_Fint *ierr)
{
}

void mpi_irecv_(void *buf, MPI_Fint *count, MPI_Fint *datatype,
                MPI_Fint *source,MPI_Fint *tag, MPI_Fint *comm,
                MPI_Fint *request,MPI_Fint *ierr)
{
}

void mpi_finalize_(MPI_Fint *ierr)
{
}
