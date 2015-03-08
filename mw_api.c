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

/*** Status Codes ***/
#define IDLE      0x00
#define BUSY      0x01

/*** Local Prototypes ***/
mw_work_t **get_next_job(mw_work_t **current_job, int count);
void KillWorkers(int n_proc);


mw_work_t **get_next_job(mw_work_t **current_job, int count) {
  int i;
  for(i=0; i<count; i++) {
    if(*current_job==NULL)break;
    current_job++;
  }
  return current_job;
}

void KillWorkers(int n_proc) {
  int i, dummy;
  MPI_Status status;
  for(i=1; i<n_proc; i++) {
    MPI_Send(&dummy, 0, MPI_INT, i, TAG_KILL, MPI_COMM_WORLD);
  }
}

void MW_Run(int argc, char **argv, struct mw_api_spec *f) {
  int rank, n_proc, i, length;
  unsigned char *send_buffer;
  unsigned char *receive_buffer;
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
    mw_result_t **next_result = result_queue;

    unsigned char worker_status[n_proc-1];
    for (i=1; i<n_proc; i++) {
      f->serialize_work(next_job, JOBS_PER_PACKET, &send_buffer, &length);
      MPI_Send(send_buffer, length, MPI_UNSIGNED_CHAR, i, TAG_COMPUTE, MPI_COMM_WORLD);
      free(send_buffer);
      next_job = get_next_job(next_job, JOBS_PER_PACKET);
      worker_status[i-1] = BUSY;
    }

    printf("Sent initial batch.\n");
    while(1) {
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &length);
      source = status.MPI_SOURCE;
      if (NULL == (receive_buffer = (unsigned char*) malloc(sizeof(unsigned char) * length))) {
        printf("malloc failed on process %d...", rank);
      };
      MPI_Recv(receive_buffer, length, MPI_UNSIGNED_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD,
            &status);
      f->deserialize_results(result_queue, receive_buffer, length);
      free(receive_buffer);
      worker_status[source-1] = IDLE;
      if (*next_job == NULL) {
        printf("Last job out\n");
        for (i=0; i<n_proc-1; i++) {
          if (worker_status[i]==BUSY) break;
        }
        if (i == (n_proc-1)) break;

        /*
        status[source-1] = IDLE;
        iterate through status; if all==IDLE, break
        */
      } else {
        f->serialize_work(next_job, JOBS_PER_PACKET, &send_buffer, &length);
        MPI_Send(send_buffer, length, MPI_UNSIGNED_CHAR, source, TAG_COMPUTE, MPI_COMM_WORLD);
        free(send_buffer);
        next_job = get_next_job(next_job, JOBS_PER_PACKET);
        worker_status[source-1] = BUSY;
      }
    }
    KillWorkers(n_proc);
    printf("Calculating result.\n");
    f->result(result_queue);
    while(*next_result!=NULL)free(*next_result++);
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
      MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &length);
      if (status.MPI_TAG == TAG_KILL)break;
      if (NULL == (receive_buffer = (unsigned char*) malloc(sizeof(unsigned char) * length))) {
        printf("malloc failed on process %d...", rank);
      };
      MPI_Recv(receive_buffer, length, MPI_UNSIGNED_CHAR, 0, TAG_COMPUTE, MPI_COMM_WORLD,
          &status);
      f->deserialize_work(work_queue, receive_buffer, length);
      free(receive_buffer);
      while(*next_job != NULL) {
        *next_result++ = f->compute(*next_job++);
        count++;
      }
      while(first_job != next_job) {
        free(*first_job);
        first_job++;
      }
      f->serialize_results(result_queue, count, &send_buffer, &length);
      MPI_Send(send_buffer, length, MPI_UNSIGNED_CHAR, 0, TAG_COMPUTE, MPI_COMM_WORLD);
      free(send_buffer);

      while(first_result != next_result) {
        free(*first_result);
        first_result++;
      }
      first_result = result_queue;
      next_result = result_queue;
      first_job = work_queue;
      next_job = work_queue;
      *next_job = NULL;
      count = 0;
    }
  }

}
