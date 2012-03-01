/*
 *  nodes.h
 *  Wrappers
 *
 *  Created by Ivan Kulagin on 09.02.11.
 */

#include "subsystem.h"

struct nodes {
    int nproc;
    int *old_proc;
};

nodes_t *subsystem_init(int *mapp, int commsize, int nparts,
                        int *pweights, int *subset_nodes)
{
    nodes_t *tmp;
    int i, j;
    
    tmp = malloc(sizeof(*tmp) * nparts);
    if (tmp == NULL) {
        return NULL;
    }
    for (i = 0; i < nparts; i++) {
        tmp[i].nproc = 0;
        tmp[i].old_proc = malloc(sizeof(int) * pweights[i]);
        if (tmp[i].old_proc == NULL) {
            for (j = 0; j < i; j++) {
                free(tmp[j].old_proc);
            }
            free(tmp);
            return NULL;
        }
    }
    fprintf(stderr, "subsystem stage 1\n");
    for (i = 0; i < commsize; i++) {
        j = subset_nodes[mapp[i]];
        tmp[j].old_proc[tmp[j].nproc] = i;
        tmp[j].nproc++;
        if (tmp[j].nproc > pweights[j]) {
            fprintf(stderr, "SUBSYSTEM ERROR\n");
        }
    }
    
    return tmp;
}
int subsystem_getproc(nodes_t *subsystem,int subset)
{
    int rc;
    rc = subsystem[subset].old_proc[subsystem[subset].nproc - 1];
    subsystem[subset].nproc--;
    return rc;
}
void subsystem_free(nodes_t *subsystem, int nparts)
{
    int i;
    
    for (i = 0; i < nparts; i++) {
        free(subsystem[i].old_proc);
    }
    free(subsystem);
}
