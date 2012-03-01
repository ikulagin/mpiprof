#include "algo.h"

void linear(int npart, int *pweights, int *new_mapp, int commsize) {
    int i, j, weight;
    
    for (i = 0, j = 0; i < commsize; i++) {
        if (weight >= pweights[j]) {
            j++;
            i--;
            weight = 0;
            continue;
        }
        new_mapp[i] = j;
        weight++;
        printf("new_mapp[%d] = %d\n", i, j);
    }
}

void rr(int npart, int *pweights, int *new_mapp, int commsize) {
    int i, j;
    int *pweights_tmp;
    
    pweights_tmp = malloc(sizeof(int) * npart);
    for (i = 0; i < npart; i++) {
        pweights_tmp[i] = pweights[i];
    }
    for (i = 0, j = 0; i < commsize; i++, j++) {
        if (j >= npart) {
            j = 0;
        }
        if (pweights_tmp[j] == 0) {
            i--;
            continue;
        }
        new_mapp[i] = j;
        pweights_tmp[j]--;
        printf("new_mapp[%d] = %d\n", i, j);
    }
    free(pweights_tmp);
}