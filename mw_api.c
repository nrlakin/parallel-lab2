/*******************************************************************************
*
* File: mw_api.c
* Description: Implementation for master-worker API.
*
*******************************************************************************/
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "mw_api.h"

void MW_Run(int argc, char **argv, struct mw_api_spec *f) {
  int rank, n_proc;

  MPI_Comm_size (MPI_COMM_WORLD, &n_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (n_proc < 2) {
    printf("Error: Master-Worker requires at least 2 processes.");
    return;
  }

  if (rank == 0) {
    mw_work_t **queue = f->create(argc, argv);


    if (f->cleanup(queue)) {
      printf("Successfully cleaned up memory.\n");
    }
  }
}
