#include "profgenmode.h"
#include "log.h"
reqlist_t *reqlist = NULL;
commtable_t *commtable = NULL;

void profgenmode_init()
{
    log_opt.output = DEVCONSOLE;
    log_opt.tracemask = TRACEMASKDEF;
    apptrace(TRACE_DBG_BIT , "INIT START\n");
    if ((mpipgo_graph = getenv("MPIPGO_GRAPH")) == NULL) {
        apptrace(TRACE_ERR_BIT, "MPIPGO_GRAPH not found\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    if ((reqlist = reqlist_create()) == NULL) {
        fprintf(stderr, "reqlist has been do not created\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    if (PMPI_Comm_size(MPI_COMM_WORLD, &profgen_commsize) != MPI_SUCCESS) {
        fprintf(stderr, "profgen_commsize\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    if (PMPI_Comm_rank(MPI_COMM_WORLD, &profgen_rank) != MPI_SUCCESS) {
        fprintf(stderr, "profgen_rank\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }    
    
    if ((commtable = commtable_create(profgen_commsize)) == NULL) {
        fprintf(stderr, "commtable\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    fprintf(stderr, "INIT FINISH\n");
}

void profgenmode_reg_pt2pt(int count, int partner, MPI_Datatype type,
                           MPI_Request req)
{
    if (reqlist_add( count, partner, type, req, reqlist) != 1) {
        fprintf(stderr, "reqlist_add error\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
}

void profgenmode_complete_pt2pt_one(MPI_Request req)
{
    request_t *tmp;
    if ((tmp = reqlist_lookup(req, reqlist)) != NULL) {
        reqlist_elem_complate(&tmp, commtable);
/*        tmp = reqlist_lookup(req, reqlist);
        if (tmp == NULL) {
            printf("tmp = NULL\n");
        }
*/
    }
}

void profgenmode_complete_pt2pt_all(MPI_Request *req, int count)
{
    int i;
    
    for (i = 0; i < count; i++) {
        profgenmode_complete_pt2pt_one(req[i]);
    }
}

void profgenmode_finalize()
{
    int *vertices, nlink, i, j, nedges, **adjmatr;
    uint64_t *weight;
    char symb;
    FILE *f, *tmp_f;
    
    printf("FINALIZE\n");
    commtable_print(profgen_rank, commtable);
    vertices = malloc(sizeof(*vertices) * profgen_commsize);
    weight = malloc(sizeof(weight) * profgen_commsize);
    if (vertices == NULL || weight == NULL) {
        fprintf(stderr, "vertices | weight error\n");
        PMPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    commtable_fill_vectors(commtable, vertices, weight, &nlink);
    if (profgen_rank != 0) {
        PMPI_Send(&nlink, 1, MPI_INT, 0, profgen_rank, MPI_COMM_WORLD);
        PMPI_Send(vertices, nlink, MPI_INT, 0, profgen_rank * 10,
                  MPI_COMM_WORLD);
        PMPI_Send(weight, nlink, MPI_UINT64_T, 0, profgen_rank * 100,
                 MPI_COMM_WORLD);
    } else {
        adjmatr = malloc(sizeof(*adjmatr) * profgen_commsize);
        for (i = 0; i < profgen_commsize; i++) {
            adjmatr[i] = malloc(sizeof(*adjmatr[i]) * profgen_commsize);
            bzero(adjmatr[i], sizeof(*adjmatr[i]) * profgen_commsize);
        }
        
        if ((tmp_f = fopen(".tmp", "w+")) != NULL) {
            for (i = 0; i < nlink; i++) {
                adjmatr[0][vertices[i] - 1] = 1;
                fprintf(tmp_f, "%d %" PRIu64 " ", vertices[i], weight[i]);
            }
            fprintf(tmp_f, "\n");
            for (i = 1; i < profgen_commsize; i++) {
                bzero(vertices, sizeof(*vertices) * profgen_commsize);
                bzero(weight, sizeof(*weight) * profgen_commsize);
                
                if (MPI_Recv(&nlink, 1, MPI_INT, i, i, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                    break;
                }
                
                if (MPI_Recv(vertices, nlink, MPI_INT, i, i * 10,
                             MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                    break;
                }
                
                if (MPI_Recv(weight, nlink, MPI_UNSIGNED_LONG, i, i * 100,
                             MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                    break;
                }
                
                for (j = 0; j < nlink; j++) {
                    adjmatr[i][vertices[j] - 1] = 1;
                    fprintf(tmp_f, "%d %" PRIu64 " ", vertices[j],
                            weight[j]);
                }
                fprintf(tmp_f, "\n");                    
            }
            /*подсчет коллиства ребер*/
            for (i = 0, nedges = 0; i < profgen_commsize; i++) {
                for (j = 0; j < i; j++) {
                    if (adjmatr[i][j] == 1) {
                        nedges++;
                    }
                }
            }
            
            if ((f = fopen(mpipgo_graph, "w+")) != NULL) {
                fseek(tmp_f, 0, SEEK_SET);
                fprintf(f, "%d %d 11\n", profgen_commsize, nedges);
                while (!feof(tmp_f)) {
                    fscanf(tmp_f, "%c", &symb);
                    fprintf(f, "%c", symb);
                }
            }
            fclose(f);
            fclose(tmp_f);
            for (i =0; i < profgen_commsize; i++) {
                free(adjmatr[i]);
            }
            free(adjmatr);
            remove(".tmp");
        }    
    }

    
    free(vertices);
    free(weight);
}
