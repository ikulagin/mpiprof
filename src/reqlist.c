#include "reqlist.h"

struct request {
    int status;
    int size;
    int partner;
    MPI_Request req;
    request_t *next;
    request_t *prev;
};

struct reqlist {
    int length;
    request_t *head;
};


reqlist_t *reqlist_create()
{
    reqlist_t *p;
    if ((p = malloc(sizeof(*p))) == NULL) {
        return NULL;
    }
    p->length = 0;
    p->head = NULL;
//    p->head;
    return p;
}

int reqlist_add(int count, int partner, MPI_Datatype datatype,
                MPI_Request req, reqlist_t *list)
{
    request_t *p, *next_p;
    int type_size;
    
    next_p = list->head;
    if ((p = malloc(sizeof(*p))) == NULL) {
        return -1;
    }
    
    p->partner = partner;
    
    MPI_Type_size(datatype, &type_size);
    p->status = REQ_NOT_CONFIRMED;
    p->size = count * type_size;
    p->req  = req;
    
    p->next = next_p;
    if (next_p != NULL) {
        next_p->prev = p;
    }
    list->head = p;
    list->head->prev = NULL;
    return 1;
}

request_t *reqlist_lookup(MPI_Request req, reqlist_t *list)
{
    request_t *tmp = list->head;
    while (tmp != NULL) {
        if (tmp->req == req) {
            return tmp;
        }
        tmp = tmp->next;
    }
    
    return NULL;
}

void reqlist_elem_complate(request_t **elem, commtable_t *table)
{
    if (*elem != NULL) {
        (*elem)->status = REQ_CONFIRMED;
        commtable_add_msgsize((*elem)->partner, (*elem)->size, table);
//        if ((*elem)->prev == NULL && (*elem)->next != NULL) { /*first in list*/
//            (*elem)->next->prev = NULL;
//            free(*elem);
//            *elem = NULL;
//            return;
//        }
//        if ((*elem)->prev != NULL && (*elem)->next == NULL) { /*last in list*/
//            (*elem)->prev->next = NULL;
//            free(*elem);
//            *elem = NULL;
//            return;
//        }
//        if ((*elem)->prev == NULL && (*elem)->next == NULL) { /*if only one element*/
//            free(*elem);
//            *elem = NULL;
//            return;
//        }
    }
}

void reqlist_fill_commtable(reqlist_t *list, commtable_t *table)
{
    request_t *tmp = list->head;
    
    while (tmp != NULL) {
        commtable_add_msgsize(tmp->partner, tmp->size, table);
        tmp = tmp->next;
    }
}

void reqlist_print(reqlist_t *list)
{
    request_t *tmp = list->head;
    
    while (tmp != NULL) {
        printf("PARTNER = %d\t\tSIZE = %d\tCONFIRMATION = %d\t",
               tmp->partner, tmp->size, tmp->status);
        tmp = tmp->next;
    }
}
