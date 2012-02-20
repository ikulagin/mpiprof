/*
 *  mapping.c
 *  BT_Wrappers
 *
 *  Created by Ivan Kulagin on 08.05.11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "mapping.h"

int getnodeid()
{
    int nodeid, i;
    char node[50], c;
    gethostname(node, sizeof(char) * LENGTH_HOSTNAME);   
    /* @mk: магические константы (50) использовать нельзя, лучше создать enum */
    
    for (i = 4; c <= '9' && c >=0; i++) { /* @mk: здесь неявное предположени, что код 
                                           символа '8' < кода символа '9'. 
                                           Так лучше не делать.*/
        c = node[i];
        if (c <= '9' && c >=0) {
            nodeid = nodeid * (i-4) + atoi(&c);
        } else {
            break;
        }
        
    }
    
    return nodeid;
}

int mapping_initialize(int commsize)
{
    int i = 0, j, max;
    
    for (i = 0, max = 0; i < commsize; i++) {
        if (old_mapp[i] > max) {
            max = old_mapp[i];
        }
    }
    subset_nodes = malloc(sizeof(int) * (max + 1));
    if (subset_nodes == NULL) {
        return -1;
    }
    for (i = 0; i <= max; i++) {
        subset_nodes[i] = 0;
    }
    
    for (i = 0, npart = 0; i < commsize; i++) {
        if (subset_nodes[old_mapp[i]] == 0)
            npart++;
        subset_nodes[old_mapp[i]]++;
    }
    
    pweights = malloc(sizeof(int) * npart);
    if (pweights == NULL) {
        return -1;
    }
    
    for (i = 0, j = 0; i <= max; i++) {
        if (subset_nodes[i] != 0) {
            pweights[j] = subset_nodes[i];
            subset_nodes[i] = j;
            j++;
        }
    }
    
    return 0;
}
int maping_allocate(int commsize, char *g, int *ranks, char *mpipgo_algo)
{
    int i = 0;
    csrgraph_t *graph;
    
    graph = csrgraph_load(g);
    if (graph == NULL) {
        fprintf(stderr, "gpart load error\n");
        return -1;
    }
/*    for (i = 0; i < commsize; i++) {
        printf("subset_nodes[%d] = %d\n", old_mapp[i],
               subset_nodes[old_mapp[i]]);
    }
    for (i = 0; i < npart; i++) {
        printf("pweights[%d] = %d\n", i, pweights[i]);
    }
*/
    if (strcmp(mpipgo_algo, "gpart") == 0) {
        if (gpart_partition_recursive(graph, pweights, npart, new_mapp) > 0) {
            fprintf(stderr, "gpart partition error\n");
            return -1;
        }
    } else if (strcmp(mpipgo_algo, "linear") == 0) {
        linear(npart, pweights, new_mapp, commsize);
    } else if (strcmp(mpipgo_algo, "rr") == 0) {
        rr(npart, pweights, new_mapp, commsize);
    }
/*    
    for (i = 0; i < commsize; i++) {
        printf("old[%d] = %d || new[%d] = %d \n", i, old_mapp[i], i,
               new_mapp[i]);
    }
*/
    subsystem = subsystem_init(old_mapp, commsize, npart, pweights,
                               subset_nodes);
    if (subsystem == NULL) {
        return -1;
    }
//    printf("stage 1\n");
    for (i = 0; i < commsize; i++) {
        ranks[subsystem_getproc(subsystem, new_mapp[i])] = i;
    }
/*    for (i = 0; i < commsize; i++) {
        printf("ranks[%d] = %d\n", i, ranks[i]);
    }
*/
    subsystem_free(subsystem, npart);

    return 0;
}
void primary_mapp(int rank, int commsize)
{
    int my_id;
    
    old_mapp = malloc(sizeof(*old_mapp) * commsize);
    new_mapp = malloc(sizeof(*old_mapp) * commsize);
    if (new_mapp == NULL || old_mapp == NULL) {
        fprintf(stderr, "new_mapping error or old_mapping\n");
        PMPI_Abort(MPI_COMM_WORLD, -1);
    }
    
    my_id = getnodeid();
    if (rank == 0) {
        PMPI_Gather(&my_id, 1, MPI_INT, old_mapp, 1,
                    MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        PMPI_Gather(&my_id, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

void mapping_free()
{
    free(old_mapp);
    free(new_mapp);
    free(subset_nodes);
    free(pweights);
}
