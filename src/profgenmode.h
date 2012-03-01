/*
 * profgenmode.h: module for creating graph
 * Created by Ivan Kulagin. <kadet901@gmail.com>
 */
#ifndef PROFGEN_MODE_
#define PROFGEN_MODE_

#include <stdio.h>
#include <strings.h>

#include <mpi.h>

#include "reqlist.h"
#include "communication.h"

/*reqlist_t *reqlist = NULL;*/
/*commtable_t *commtable = NULL;*/

char *mpipgo_graph;
int profgen_commsize;
int profgen_rank;

/* 
 * profgenmode_init: Funtion initializes profgen mode.
 */
void profgenmode_init();
/* 
 * profgenmode_reg_pt2pt: Function adds request to list of requests.
 */
void profgenmode_reg_pt2pt(int count, int partner, MPI_Datatype type,
                           MPI_Request req);
/*
 * profgenmode_complete_pt2pt_one: Function to complete pt2pt operation for 
 * one request.
 */
void profgenmode_complete_pt2pt_one(MPI_Request req);
/*
 * profgenmode_complete_pt2pt_one: Function to complete pt2pt operation for 
 * all request.
 */
void profgenmode_complete_pt2pt_all(MPI_Request *req, int count);

void profgenmode_finalize();

#endif /*PROFGEN_MODE_*/
