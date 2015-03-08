#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mw_api.h"

#define VECTOR_LENGTH 10
#define N_JOBS  3

struct userdef_work_t {
  int length;
  double *vector;
};

struct userdef_result_t {
  double product;
};

void printVector(double * vec, int len) {
  int i;
  if(len > 10) return;
  printf("Vector = %X\n", (unsigned int)vec);
  printf("Got:\n");
  for (i=0; i<len; i++) {
    printf("%d: %f\n", i, *vec++);
  }
}

int serialize_jobs(struct userdef_work_t **start_job, int n_jobs, unsigned char **array, int *len) {
  int i, length=0;
  unsigned char *destPtr;
  long job_len;
  struct userdef_work_t **job = start_job;

  for(i = 0; i < n_jobs; i++) {
    if(*job == NULL)break;
    length += (sizeof(double) * (*job)->length) + sizeof(long);
    job++;
  }
  if (NULL == (*array = (unsigned char*)malloc(sizeof(unsigned char) * length))) {
    printf ("malloc failed on send buffer...");
    return 0;
  };
  *len = length;
  job = start_job;
  destPtr = *array;
  for(i = 0; i < n_jobs; i++) {
    if(*job == NULL) break;
    job_len = (*job)->length;
    destPtr = memcpy(destPtr, &job_len, sizeof(long));
    destPtr += sizeof(long);
    destPtr = memcpy(destPtr, (*job)->vector, sizeof(double) * (*job)->length);
    destPtr += sizeof(double) * (*job)->length;
    job++;
  }
  return 1;
}

int deserialize_jobs(struct userdef_work_t **queue, unsigned char *array, int len) {
  struct userdef_work_t *jobPtr;
  unsigned char *srcPtr = array;
  long templong;
  while (NULL != *queue)queue++;
  while(len) {
    if (NULL == (jobPtr = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t)))) {
      printf ("malloc failed on receive buffer...");
      return 0;
    };
    memcpy(&templong, srcPtr, sizeof(long));
    srcPtr += sizeof(long);
    jobPtr->length = templong;
    if (NULL == ((jobPtr->vector) = (double*)malloc(sizeof(double) * jobPtr->length))) {
      printf ("malloc failed on receive buffer...");
      return 0;
    };
    memcpy(jobPtr->vector, srcPtr, jobPtr->length * sizeof(double));
    srcPtr += jobPtr->length * sizeof(double);
    *queue++ = jobPtr;
    len-=(jobPtr->length * sizeof(double)) + sizeof(long);
  }
  *queue = NULL;
  return 1;
}

struct userdef_work_t **create_jobs (int argc, char **argv) {
  int i, chunk_size;
  const unsigned int VectorLength = VECTOR_LENGTH; // in case VECTOR_LENGTH is expression
  double *vector;
  struct userdef_work_t **job_queue;
  struct userdef_work_t *jobs;

  if (NULL == (vector = (double*)malloc(sizeof(double) * VectorLength))) {
    printf ("malloc failed on allocating vector...");
    return NULL;
  };
  if (NULL == (jobs = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t) * N_JOBS))) {
    printf ("malloc failed on allocating jobs...");
    return NULL;
  };
  if (NULL == (job_queue = (struct userdef_work_t**)malloc(sizeof(struct userdef_work_t*) * N_JOBS + 1))) {
    printf ("malloc failed on allocating job queue...");
    return NULL;
  };

  srand(time(NULL));
  for (i = 0; i < VectorLength; i++) {
    vector[i] = (double)rand() / RAND_MAX;
  }

  printf("Generated vector of length %d.\n", i);
  printVector(vector, i);

  chunk_size = VectorLength / N_JOBS;

  for (i = 0; i < N_JOBS; i++) {
    jobs[i].length = chunk_size;
    if (i == N_JOBS-1) {
      jobs[i].length += VectorLength % N_JOBS;
    }
    jobs[i].vector = &vector[i*chunk_size];
    job_queue[i] = &(jobs[i]);
  }
  job_queue[i] = NULL;

  return job_queue;
}

int serialize_results(struct userdef_result_t **start_result, int n_results, unsigned char **array, int *len) {
  struct userdef_result_t **result = start_result;
  int i, length=0;
  unsigned char *destPtr;

  for(i = 0; i < n_results; i++) {
    if(*result == NULL)break;
    length += sizeof(double);
    result++;
  }
  if (NULL == (*array = (unsigned char*)malloc(sizeof(unsigned char) * length))) {
    printf ("malloc failed on send buffer...\n");
    return 0;
  };
  *len = length;
  result = start_result;
  destPtr = *array;
  for(i = 0; i < n_results; i++) {
    if(*result == NULL) break;
    memcpy(destPtr, &((*result)->product), sizeof(double));
    destPtr += sizeof(double);
    result++;
  }
  return 0;
}

int deserialize_results(struct userdef_result_t **queue, unsigned char *array, int len) {
  struct userdef_result_t *resultPtr;
  unsigned char *srcPtr = array;
  while (NULL != *queue)queue++;
  while(len) {
    if (NULL == (resultPtr = (struct userdef_result_t*)malloc(sizeof(struct userdef_result_t)))) {
      printf ("malloc failed on receive buffer...\n");
      return 0;
    };
    memcpy(&(resultPtr->product), srcPtr, sizeof(double));
    srcPtr += sizeof(double);
    *queue++ = resultPtr;
    len-=sizeof(double);
    printf("master received %f\n", resultPtr->product);
  }
  *queue = NULL;
  return 1;
}

int main(int argc, char **argv) {
  int i, length;
  struct userdef_work_t **work_queue;
  struct userdef_result_t *result_queue[4];
  struct userdef_work_t *rec_work[4];
  struct userdef_result_t *rec_result[4];
  unsigned char *send_buff;
  rec_work[0]=NULL;
  rec_result[0] = NULL;
  result_queue[0] = NULL;

  MPI_Init(&argc, &argv);

  work_queue = create_jobs(argc, argv);
  for (i=0; i<N_JOBS; i++) {
    printf("Job %d:\n", i+1);
    printVector(work_queue[i]->vector, work_queue[i]->length);
  }

  serialize_jobs(work_queue, N_JOBS, &send_buff, &length);
  printf("serialized length: %d\n", length);
  deserialize_jobs(rec_work, send_buff, length);
  for (i=0; i<N_JOBS; i++) {
    printf("Rec Job %d:\n", i+1);
    printVector(rec_work[i]->vector, rec_work[i]->length);
    free(rec_work[i]->vector);
    free(rec_work[i]);
  }
  free(work_queue[0]->vector);
  free(work_queue[0]);
  free(work_queue);
  free(send_buff);

  printf("Testing result functions\n");
  for (i=0; i<N_JOBS; i++) {
    printf("result %d:\n", i+1);
    if (NULL == (result_queue[i] = (struct userdef_result_t*)malloc(sizeof(struct userdef_result_t)))) {
      printf ("malloc failed on result queue...\n");
      return 0;
    };
    result_queue[i]->product = (double)i+1;
    printf("%f\n", result_queue[i]->product);
  }
  serialize_results(result_queue, 3, &send_buff, &length);
  printf("serialized_length: %d\n", length);
  deserialize_results(rec_result, send_buff, length);
  for(i=0; i<3; i++) {
    printf("Rec result %d:\n", i+1);
    printf("%f\n", rec_result[i]->product);
    free(rec_result[i]);
    free(result_queue[i]);
  }
  free(send_buff);
  MPI_Finalize();
  exit(0);
}
