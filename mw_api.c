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

#define QUEUE_LENGTH  1000
#define JOBS_PER_PACKET 5

#define TAG_COMPUTE 0
#define TAG_KILL    1

/*** Local Prototypes ***/
mw_work_t **get_next_job(mw_work_t **current_job, int count);
void KillWorkers(int n_proc);


mw_work_t **get_next_job(mw_work_t **current_job, int count) {
  int i;
  for(i=1; i<count; i++) {
    if(*current_job==NULL)break;
    current_job++;
  }
  return current_job;
}

void KillWorkers(int n_proc) {
  int i, dummy;
  for(i=1; i<n_proc; i++) {
    MPI_Send(&dummy, 0, MPI_INT, i, TAG_KILL, MPI_COMM_WORLD);
  }
}

void MW_Run(int argc, char **argv, struct mw_api_spec *f) {
  int rank, n_proc, i, length;
  double *send_buffer;
  double *receive_buffer;
  mw_result_t *result_queue[QUEUE_LENGTH+1];
  MPI_Status status;

  result_queue[0] = NULL;

  MPI_Comm_size (MPI_COMM_WORLD, &n_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (n_proc < 2) {
    printf("Error: Master-Worker requires at least 2 processes.\n");
    return;
  }

  if (rank == 0) {
    int source;
    mw_work_t **work_queue = f->create(argc, argv);
    mw_work_t **next_job = work_queue;
    for (i=1; i<n_proc; i++) {
      f->serialize_work(next_job, JOBS_PER_PACKET, &send_buffer, &length);
      MPI_Send(send_buffer, length/8, MPI_DOUBLE, i, TAG_COMPUTE, MPI_COMM_WORLD);
      free(send_buffer);
      next_job = get_next_job(next_job, JOBS_PER_PACKET);
    }

    while(1) {
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_DOUBLE, &length);
      source = status.MPI_SOURCE;
      if (NULL == (receive_buffer = (double*) malloc(sizeof(double) * length))) {
        printf("malloc failed on process %d...", rank);
      };
      MPI_Recv(receive_buffer, length, MPI_DOUBLE, source, MPI_ANY_TAG, MPI_COMM_WORLD,
            &status);
      f->deserialize_results(result_queue, receive_buffer, length*8);
      free(receive_buffer);
      if (*next_job == NULL)break;
      f->serialize_work(next_job, JOBS_PER_PACKET, &send_buffer, &length);
      MPI_Send(send_buffer, length/8, MPI_DOUBLE, source, TAG_COMPUTE, MPI_COMM_WORLD);
      free(send_buffer);
      next_job = get_next_job(next_job, JOBS_PER_PACKET);
    }
    f->result(result_queue);
    KillWorkers(n_proc);
    if (f->cleanup(work_queue)) {
      printf("Successfully cleaned up memory.\n");
    }
  } else {
    mw_work_t *work_queue[QUEUE_LENGTH+1];
    mw_work_t **first_job = work_queue, **next_job = work_queue;
    mw_result_t **first_result = result_queue, **next_result = result_queue;
    int count = 0;
    while(1) {
      MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_DOUBLE, &length);
      if (status.MPI_TAG == TAG_KILL)break;
      if (NULL == (receive_buffer = (double*) malloc(sizeof(double) * length))) {
        printf("malloc failed on process %d...", rank);
      };
      MPI_Recv(receive_buffer, length, MPI_DOUBLE, 0, TAG_COMPUTE, MPI_COMM_WORLD,
          &status);
      f->deserialize_work(work_queue, receive_buffer, length*8);
      while(*next_job != NULL) {
        *next_result++ = f->compute(*next_job++);
        count++;
      }
      free(receive_buffer);
      while(first_job != next_job)free(*first_job++);
      f->serialize_results(result_queue, count, &send_buffer, &length);
      printf("Sending results\n");
      MPI_Send(send_buffer, length/8, MPI_DOUBLE, 0, TAG_COMPUTE, MPI_COMM_WORLD);
      free(send_buffer);

      while(first_result != next_result) {
        free(*first_result++);
      }
      first_result = result_queue;
      next_result = result_queue;
      first_job = work_queue;
      next_job = work_queue;
      count = 0;
    }
  }

}
