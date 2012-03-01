/*
 *  communication.c
 *  BT_Wrappers
 *
 *  Created by Ivan Kulagin on 09.02.11.
 *
 */

#include "communication.h"

struct comm {
    int nops;
    uint64_t msgsize;
};

struct commtable {
    int comm_size;
    comm_t *process;
};

commtable_t *commtable_create(int n)
{
    commtable_t *table;
    int i;
    
    if (n <= 0) {
        return NULL;
    }
    
    if ((table = malloc(sizeof(*table))) == NULL) {
        return NULL;
    }
    
    table->comm_size = n;
    
    if ((table->process = malloc(sizeof(comm_t) * n)) == NULL) {
        free(table);
        return NULL;
    }

    for (i = 0; i < table->comm_size; i++) {
        table->process[i].msgsize = 0;
        table->process[i].nops = 0;
    }
    
    return table;
}

void commtable_add_datatype(int partner, int count, MPI_Datatype datatype,
                   commtable_t *table)
{
    int size;
    uint64_t msgsize;
    
    MPI_Type_size(datatype, &size);
    msgsize = size * count;
    
    table->process[partner].msgsize += msgsize;
    table->process[partner].nops += 1;
}

void commtable_add_msgsize(int partner, uint64_t msgsize, commtable_t *table)
{
    table->process[partner].msgsize += msgsize;
    table->process[partner].nops += 1;
}

int commtable_fill_vectors(commtable_t *table, int *vertices, uint64_t *weight,
                           int *nedges)
{
    int i = 0, j = 0;
    
    *nedges = 0;
    if (table == NULL) {
        return -1;
    }
    
    for (i = 0, j = 0; i < table->comm_size; i++) {
        if (table->process[i].msgsize != 0) {
            vertices[j] = i+1;
            weight[j] = table->process[i].msgsize;
            *nedges += 1;
            j++;
        }
    }
    
    return 1;
}

void commtable_print(int rank, commtable_t *table)
{
    int i = 0;
    for (i = 0; i < table->comm_size; i++) {
        printf("%d\t--- %d\tsize = " "%" PRIu64 "\n", rank, i, table->process[i].msgsize);
    }
}

void commtable_free(commtable_t *table)
{
    if (table != NULL) {
        if (table->comm_size > 0) {
            free(table->process);
        }
        free(table);
    }
}
