/*******************************************************************************
*
* File: mw_factor.c
* Description: Large number factoring application for mw_api.
*
*******************************************************************************/

#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mw_api.h"
#include <gmp.h>

// Must be less than max int!
#define START_NUM "10000000"
#define N_JOBS  10

struct userdef_work_t {
  mpz_t target;
  mpz_t rangeStart;
  mpz_t rangeEnd;
};

// be sure to include number it multiplies with when gathering factors
struct userdef_result_t {
  int length;
  mpz_t *factors;
};

struct userdef_work_t **create_jobs (int argc, char **argv) {
  struct userdef_work_t **job_queue;
  struct userdef_work_t *jobs;

  if (NULL == (jobs = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t) * N_JOBS))) {
    printf ("malloc failed on allocating jobs...");
    return NULL;
  };

  if (NULL == (job_queue = (struct userdef_work_t**)malloc(sizeof(struct userdef_work_t*) * N_JOBS + 1))) {
    printf ("malloc failed on allocating job queue...");
    return NULL;
  };

  int i;
  mpz_t n, root, chunk_size;

  mpz_init_set_str(n, START_NUM, 10);
  mpz_init(root);
  mpz_init(chunk_size);
  mpz_sqrt(root, n);
  mpz_tdiv_q_ui(chunk_size, root, N_JOBS);

  // need to skip 0,1 somewhere
  for (i = 0; i < N_JOBS; i++) {
    mpz_init_set(jobs[i].target, n);
    mpz_init(jobs[i].rangeStart);
    mpz_mul_si(jobs[i].rangeStart, chunk_size, i);
    mpz_init(jobs[i].rangeEnd);
    if (i == N_JOBS-1) {
      mpz_set(jobs[i].rangeEnd, root);
    } else {
      mpz_mul_si(jobs[i].rangeEnd, chunk_size, i+1);
    }
    job_queue[i] = &(jobs[i]);
  }
  job_queue[i] = NULL;
  return job_queue;
}

int serialize_jobs(struct userdef_work_t **start_job, int n_jobs, double **array, int *len) {
  int i, length=0;
  void *destPtr;
  long job_len;
  struct userdef_work_t **job = start_job;

  for(i = 0; i < n_jobs; i++) {
    if(*job == NULL)break;
    length += (sizeof(double) * (*job)->length) + sizeof(long);
    job++;
  }
  if (NULL == (*array = (double*)malloc(sizeof(double) * length))) {
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

int deserialize_jobs(struct userdef_work_t **queue, double *array, int len) {
  struct userdef_work_t *jobPtr;
  long *lengthPtr = array;
  double *srcPtr = ++array;
  while (NULL != *queue)queue++;
  while(len) {
    if (NULL == (jobPtr = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t)))) {
      printf ("malloc failed on receive buffer...");
      return 0;
    };
    jobPtr->length = *lengthPtr;
    jobPtr->vector = srcPtr;
    *queue++ = jobPtr;
    srcPtr += jobPtr->length;
    lengthPtr = srcPtr;
    srcPtr++;
    len-=(jobPtr->length * sizeof(double)) + sizeof(long);
  }
  *queue = NULL;
  return 1;
}

int serialize_results(struct userdef_result_t **start_result, int n_results, double **array, int *len) {
  struct userdef_result_t **result = start_result;
  int i, length=0;
  double *destPtr;

  for(i = 0; i < n_results; i++) {
    if(*result == NULL)break;
    length += sizeof(double);
    result++;
  }
  if (NULL == (*array = (double*)malloc(sizeof(double) * length))) {
    printf ("malloc failed on send buffer...");
    return 0;
  };
  *len = length;
  result = start_result;
  destPtr = *array;
  for(i = 0; i < n_results; i++) {
    if(*result == NULL) break;
    *destPtr++ = (*result)->product;
    result++;
  }
  return 0;
}

int deserialize_results(struct userdef_result_t **queue, double *array, int len) {
  struct userdef_result_t *resultPtr;
  double *srcPtr = array;
  while (NULL != *queue)queue++;
  while(len) {
    if (NULL == (resultPtr = (struct userdef_result_t*)malloc(sizeof(struct userdef_result_t)))) {
      printf ("malloc failed on receive buffer...");
      return 0;
    };
    resultPtr->product = *srcPtr;
    *queue++ = resultPtr;
    srcPtr++;
    len-=sizeof(double);
  }
  *queue = NULL;
  return 1;
}

int userdef_result (struct userdef_result_t **res) {
  double dot_product = 0;
  int i;

  while(*res != NULL) {
    dot_product += (*res)->product;
    res++;
  }
  printf("Dot product: %f\n", dot_product);
  return 1;
}

struct userdef_result_t *userdef_compute (struct userdef_work_t *work) {
  struct userdef_result_t *result;
  int i;
  if (NULL == (result = (struct userdef_result_t*)malloc(sizeof(struct userdef_result_t)))) {
    printf ("malloc failed on userdef_compute...");
    return NULL;
  };
  printf("Worker got job:\n");
  printVector(work->vector, work->length);
  result->product = norm(work->vector, work->length);
  return result;
}

int cleanup(struct userdef_work_t **work) {
  free(work[0]);
  free(work);
  return 1;
}

int main(int argc, char **argv) {
  struct mw_api_spec f;
  struct userdef_work_t **master_queue;
  struct userdef_work_t *worker_queue[50];
  double *receive_buffer;
  int length, i;
  double *value;

  MPI_Init(&argc, &argv);

  f.create = create_jobs;
  f.result = userdef_result;
  f.compute = userdef_compute;
  f.serialize_work = serialize_jobs;
  f.deserialize_work = deserialize_jobs;
  f.serialize_results = serialize_results;
  f.deserialize_results = deserialize_results;
  f.cleanup = cleanup;
  f.work_sz = sizeof(struct userdef_work_t);
  f.res_sz = sizeof(struct userdef_result_t);

  MW_Run(argc, argv, &f);

  MPI_Finalize();

  return 0;
}
