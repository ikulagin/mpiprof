
#include "profusemode.h"

MPI_Comm newcomm;
MPI_Group worldgrouup, newgroup;

int mpiprofuse_init(int *argc, char ***argv)
{
    int *ranks, old_rank;
    
    if(PMPI_Comm_size(MPI_COMM_WORLD, &profuse_commsize) != MPI_SUCCESS) {
        fprintf(stderr, "profuse_commsize\n");
        PMPI_Abort(MPI_COMM_WORLD, -1);
    }
    if(PMPI_Comm_rank(MPI_COMM_WORLD, &profuse_rank) != MPI_SUCCESS) {
        fprintf(stderr, "profuse_rank\n");
        PMPI_Abort(MPI_COMM_WORLD, -1);
    }
    if ((mpipgo_graph = getenv("MPIPGO_GRAPH")) == NULL) {
        fprintf(stderr, "MPIPGO_GRAPH\n");
        PMPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    if ((mpipgo_algo = getenv("MPIPGO_ALGO")) == NULL) {
        fprintf(stderr, "MPIPGO_ALGO\n");
        PMPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((ranks = malloc(sizeof(int) * profuse_commsize)) == NULL) {
        fprintf(stderr, "profuse ranks error\n");
        PMPI_Abort(MPI_COMM_WORLD, -1);
    }

    mapp_begin = MPI_Wtime();
    
    primary_mapp(profuse_rank, profuse_commsize);
    if (profuse_rank == 0) {
        mapping_initialize(profuse_commsize);
        if(maping_allocate(profuse_commsize, mpipgo_graph, ranks,
                           mpipgo_algo) != 0) {
            fprintf(stderr, "mapping allocate error\n");
            PMPI_Abort(MPI_COMM_WORLD, -1);
        }
        mapping_free();
    }
    PMPI_Bcast(ranks, profuse_commsize, MPI_INT, 0, MPI_COMM_WORLD);
    PMPI_Comm_group(MPI_COMM_WORLD, &worldgrouup);
    PMPI_Group_incl(worldgrouup, profuse_commsize, ranks, &newgroup);
    PMPI_Comm_create(MPI_COMM_WORLD, newgroup, &newcomm);
    free(ranks);
    PMPI_Group_free(&worldgrouup);
    old_rank = profuse_rank;
    PMPI_Comm_rank(newcomm, &profuse_rank);
//    printf("Now process %d is %d/%d\n",old_rank, profuse_rank,
//           profuse_commsize);
    PMPI_Barrier(newcomm);
    if (profuse_rank == 0) {
        mapp_end = MPI_Wtime();
        test_begin = MPI_Wtime();
    }
    return MPI_SUCCESS;
}
int mpiprofuse_comm_size(MPI_Comm comm, int *size)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT ) {
        rc = MPI_Comm_size(newcomm, size);
    } else {
        rc = MPI_Comm_size( comm, size );
    }
    
    return rc;
}

int mpiprofuse_comm_rank(MPI_Comm comm, int *rank)
{
    int rc;
    
    MPI_Comm_compare(comm, MPI_COMM_WORLD, &rc);
    if (rc != MPI_IDENT) {
        rc = MPI_Comm_rank(comm, rank);
    } else {
        rc = MPI_Comm_rank(newcomm, rank);
    }
    
    return rc;
}

int mpiprofuse_abort(MPI_Comm comm, int errcode)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Abort(newcomm, errcode);
    } else {
        rc = MPI_Abort(comm, errcode);
    }
    
    return rc;
}

int mpiprofuse_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm_)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Comm_split(newcomm, color, key, newcomm_);
    } else {
        rc = MPI_Comm_split(comm, color, key, newcomm_);
    }
    
    return rc;
}

int mpiprofuse_comm_dup(MPI_Comm comm, MPI_Comm *newcomm_)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Comm_dup(newcomm, newcomm_);
    } else {
        rc = MPI_Comm_dup(comm, newcomm_);
    }
    
    return rc;
}

int mpiprofuse_reduce(void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, newcomm);
    } else {
        rc = MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    }
    
    return rc;
}

int mpiprofuse_allreduce(void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, newcomm);
    } else {
        rc = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    }
    
    return rc;
}

int mpiprofuse_isend(void *buf, int count, MPI_Datatype datatype, int dest,
                  int tag, MPI_Comm comm, MPI_Request *request)
{
    int  rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Isend(buf, count, datatype, dest, tag, newcomm, request);
    } else {
        rc = MPI_Isend(buf, count, datatype, dest, tag, comm, request);
    }
    
    return rc;
}

int mpiprofuse_irecv(void *buf, int count, MPI_Datatype datatype, int source,
                  int tag, MPI_Comm comm, MPI_Request *request)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Irecv(buf, count, datatype, source, tag, newcomm, request);
    } else {
        rc = MPI_Irecv(buf, count, datatype, source, tag, comm, request);
    }
    
    return rc;
}

int mpiprofuse_barrier(MPI_Comm comm)
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Barrier(newcomm);
    } else {
        rc = MPI_Barrier(comm);
    }
    
    return rc;
}

int mpiprofuse_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                  MPI_Comm comm )
{
    int rc;
    
    MPI_Comm_compare(MPI_COMM_WORLD, comm, &rc);
    if (rc == MPI_IDENT) {
        rc = MPI_Bcast(buffer, count, datatype, root, newcomm);
    } else {
        rc = MPI_Bcast(buffer, count, datatype, root, comm);
    }
    
    return rc;
}

int mpiprofuse_finalize()
{
    int rc;
    
    PMPI_Barrier(newcomm);
    if (profuse_rank == 0) {
        test_end = MPI_Wtime();
        printf("mapp begin = %f\n"
               "mapp end = %f\n", mapp_begin, mapp_end);
        printf("time mapping = %f\n", (mapp_end - mapp_begin));
        printf("time test = %f\n", (test_end - test_begin));
    }
    rc = PMPI_Finalize();
    
    return rc;
}
