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

#define WORKER_QUEUE_LENGTH 1000
#define MASTER_QUEUE_LENGTH 10000
//#define JOBS_PER_PACKET 5

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

void SendWork(int dest, mw_work_t **first_job, int n_jobs, struct mw_api_spec *f) {
  unsigned char *send_buffer;
  int length;
  f->serialize_work(first_job, n_jobs, &send_buffer, &length);
  MPI_Send(send_buffer, length, MPI_UNSIGNED_CHAR, dest, TAG_COMPUTE, MPI_COMM_WORLD);
  free(send_buffer);
}

void SendResults(int dest, mw_result_t **first_result, int n_results, struct mw_api_spec *f) {
  unsigned char *send_buffer;
  int length;
  f->serialize_results(first_result, n_results, &send_buffer, &length);
  MPI_Send(send_buffer, length, MPI_UNSIGNED_CHAR, 0, TAG_COMPUTE, MPI_COMM_WORLD);
  free(send_buffer);
}

void MW_Run(int argc, char **argv, struct mw_api_spec *f) {
  int rank, n_proc, i, length;
  unsigned char *receive_buffer;
  MPI_Status status;

  MPI_Comm_size (MPI_COMM_WORLD, &n_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  if (n_proc < 2) {
    printf("Error: Master-Worker requires at least 2 processes.\n");
    return;
  }

  if (rank == 0) {
    int source;

    // Init work queues
    mw_work_t **work_queue = f->create(argc, argv);
    mw_work_t **next_job = work_queue;
    mw_result_t *result_queue[MASTER_QUEUE_LENGTH];
    mw_result_t **next_result = result_queue;
    result_queue[0] = NULL;

    unsigned char worker_status[n_proc-1];
    for (i=1; i<n_proc; i++) {
      SendWork(i, next_job, f->jobs_per_packet, f);
      next_job = get_next_job(next_job, f->jobs_per_packet);
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
        for (i=0; i<n_proc-1; i++) {
          if (worker_status[i]==BUSY) break;
        }
        if (i == (n_proc-1)) break;
      } else {
        SendWork(source, next_job, f->jobs_per_packet, f);
        next_job = get_next_job(next_job, f->jobs_per_packet);
        worker_status[source-1] = BUSY;
      }
    }
    KillWorkers(n_proc);
    printf("Calculating result.\n");
    f->result(result_queue);

    if (f->cleanup(work_queue, result_queue)) {
      printf("Successfully cleaned up memory.\n");
    }
  } else {
    mw_work_t *work_queue[WORKER_QUEUE_LENGTH];
    mw_work_t **next_job = work_queue;
    mw_result_t *result_queue[WORKER_QUEUE_LENGTH];
    mw_result_t **next_result = result_queue;
    work_queue[0] = NULL;
    result_queue[0] = NULL;
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
      next_job = work_queue;
      while(*next_job != NULL) free(*next_job++);
      SendResults(0, result_queue, count, f);
      next_result = result_queue;
      while(*next_result != NULL) free(*next_result++);

      // Reset pointers
      next_result = result_queue;
      next_job = work_queue;
      *next_job = NULL;
      *next_result = NULL;
      count = 0;
    }
  }

}
