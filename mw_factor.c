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
#include "./mw_api.h"
#include <gmp.h>

// Must be less than max int!
#define START_NUM "10000"
#define N_JOBS  4

void printFactors(unsigned long * vec, unsigned long len) {
  int i;
  printf("Printing Vector:\n");
  for (i=0; i<len; i++) {
    printf("%lu: %lu\n", i, *vec++);
  }
}

struct userdef_work_t {
  mpz_t target;
  unsigned long rangeStart;
  unsigned long rangeEnd;
};

struct userdef_result_t {
  unsigned long length;
  unsigned long *factors;
};

struct userdef_work_t **create_jobs(int argc, char **argv) {
  struct userdef_work_t **job_queue;
  struct userdef_work_t *jobs;

  if (NULL == (jobs = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t) * N_JOBS))) {
    printf("malloc failed on allocating jobs...");
    return NULL;
  }

  if (NULL == (job_queue = (struct userdef_work_t**)malloc(sizeof(struct userdef_work_t*) * N_JOBS + 1))) {
    printf("malloc failed on allocating job queue...");
    return NULL;
  }

  int i;
  unsigned long root_ul, chunk_size_ul;
  mpz_t n, root, chunk_size;

  // need to clear n
  mpz_init_set_str(n, START_NUM, 10);
  mpz_init(root);
  mpz_init(chunk_size);
  mpz_sqrt(root, n);
  mpz_tdiv_q_ui(chunk_size, root, N_JOBS);
  // at most root can be max_ul-1
  root_ul = mpz_get_ui(root);
  chunk_size_ul = mpz_get_ui(chunk_size);

  for (i = 0; i < N_JOBS; i++) {
    mpz_init_set(jobs[i].target, n);
    jobs[i].rangeStart = chunk_size_ul * i;
    if (i == N_JOBS-1) {
      jobs[i].rangeEnd = root_ul + 1;
    } else {
      jobs[i].rangeEnd = (chunk_size_ul * (i+1));
    }
    job_queue[i] = &(jobs[i]);
    printf("Start: %lu\n", jobs[i].rangeStart);
    printf("End: %lu\n", jobs[i].rangeEnd);
    printf("Chunk: %lu\n", chunk_size_ul);
  }
  job_queue[i] = NULL;

  mpz_clear(root);
  mpz_clear(chunk_size);
  return job_queue;
}

int userdef_result(struct userdef_result_t **res) {
  printf("Received Results:\n");
  while(*res != NULL) {
    printFactors((*res)->factors, (*res)->length);
    res++;
  }
  return 1;
}

unsigned long getFactorLength(mpz_t target, unsigned long start, unsigned long end) {
  unsigned long length = 0;
  mpz_t mod;
  unsigned long current;
  mpz_init(mod);
  for (current = start; current < end; current++) {
    if (!(current == 0) && !(current == 1)) {
      mpz_mod_ui(mod, target, current);
      if (mpz_cmp_ui(mod, 0) == 0) {
        length += 2;
      }
    }
  }
  mpz_clear(mod);
  printf("Length: %lu\n", length);
  return length;
}

unsigned long *getFactors(mpz_t target, unsigned long start, unsigned long end, unsigned long length) {
  int count = 0;
  unsigned long current;
  unsigned long *factors;
  mpz_t mod, factor;
  mpz_init(mod);
  mpz_init(factor);
  if (NULL == (factors = (unsigned long*)malloc(sizeof(unsigned long) * length))) {
    printf ("malloc failed on allocating factors array...");
    return NULL;
  };

  for (current = start; current < end; current++) {
    if (!(current == 0) && !(current == 1)) {
      mpz_mod_ui(mod, target, current);
      if (mpz_cmp_ui(mod, 0) == 0) {
        factors[count++] = current;
        mpz_tdiv_q_ui(factor, target, current);
        factors[count++] = mpz_get_ui(factor);
      }
    }
  }
  mpz_clear(mod);
  mpz_clear(factor);
  return factors;
}

struct userdef_result_t *userdef_compute(struct userdef_work_t *work) {
  struct userdef_result_t *result;
  unsigned long length = getFactorLength(work->target, work->rangeStart, work->rangeEnd);
  if (NULL == (result = (unsigned long*)malloc(sizeof(unsigned long) * (length + 1)))) {
    printf("malloc failed on userdef_result...");
    return NULL;
  }
  printf("Worker got job:\n");
  result->length = length;
  printf("Length: %lu\n", result->length);
  result->factors = getFactors(work->target, work->rangeStart, work->rangeEnd, result->length);
  // printFactors(result->factors, result->length);
  return result;
}

int cleanup(struct userdef_work_t **work) {
  mpz_clear(work[0]->target);
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
