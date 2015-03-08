/*******************************************************************************
*
* File: mw_test.c
* Description: Test application for mw_api.
*
*******************************************************************************/

#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mw_api.h"

// Must be less than max int!
#define VECTOR_LENGTH 100000000
#define N_JOBS  1000
//#define VECTOR_LENGTH 5

/***
Given pointer to vector of doubles and vector length, calculate L2 norm
by taking dot product with itself.
***/
double norm(double* vec, int len) {
  int i;
  double result = 0;
  for (i=0; i<len; i++) {
    result += (*vec)*(*vec);
    vec++;
  }
  return result;
}

// Debug function to print short vectors to console.
void printVector(double * vec, int len) {
  int i;
  if(len > 10) return;
  printf("Vector = %X\n", (unsigned int)vec);
  printf("Got:\n");
  for (i=0; i<len; i++) {
    printf("%d: %f\n", i, *vec++);
  }
}

struct userdef_work_t {
  int length;
  double *vector;
};

struct userdef_result_t {
  double product;
};

struct userdef_work_t **create_jobs (int argc, char **argv) {
  int i, chunk_size;
  const unsigned int VectorLength = VECTOR_LENGTH; // in case VECTOR_LENGTH is expression
  double *vector;
  struct userdef_work_t **job_queue;
  struct userdef_work_t *jobs;

  if (NULL == (vector = (double*)malloc(sizeof(double) * VectorLength))) {
    printf ("malloc failed on allocating vector...\n");
    return NULL;
  };
  if (NULL == (jobs = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t) * N_JOBS))) {
    printf ("malloc failed on allocating jobs...\n");
    return NULL;
  };
  if (NULL == (job_queue = (struct userdef_work_t**)malloc(sizeof(struct userdef_work_t*) * N_JOBS + 1))) {
    printf ("malloc failed on allocating job queue...\n");
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
    printf ("malloc failed on send buffer...\n");
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
      printf ("malloc failed on alloc job struct...\n");
      return 0;
    };
    memcpy(&templong, srcPtr, sizeof(long));
    srcPtr += sizeof(long);
    jobPtr->length = templong;
    if (NULL == ((jobPtr->vector) = (double*)malloc(sizeof(double) * jobPtr->length))) {
      printf ("malloc failed on allocating vector of len %d\n...", jobPtr->length);
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
    memcpy((unsigned char*)&(resultPtr->product), srcPtr, sizeof(double));
    srcPtr += sizeof(double);
    *queue++ = resultPtr;
    len-=sizeof(double);
  }
  *queue = NULL;
  return 1;
}

int dot_product_result (struct userdef_result_t **res) {
  double dot_product = 0;
  int i;

  while(*res != NULL) {
    dot_product += (*res)->product;
    res++;
  }
  printf("Dot product: %f\n", dot_product);
  return 1;
}

struct userdef_result_t *compute_dot (struct userdef_work_t *work) {
  struct userdef_result_t *result;
  int i;
  if (NULL == (result = (struct userdef_result_t*)malloc(sizeof(struct userdef_result_t)))) {
    printf ("malloc failed on compute_dot...");
    return NULL;
  };
  printVector(work->vector, work->length);
  result->product = norm(work->vector, work->length);
  free(work->vector);
  return result;
}

int cleanup(struct userdef_work_t **work) {
  free(work[0]->vector);
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
  f.result = dot_product_result;
  f.compute = compute_dot;
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
  /*
  int n_proc, rank, i, chunk_size, msg_len, tag=1;
  MPI_Status status;
  double *vector;
  double result, start, end;

  MPI_Init(&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &n_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  // Initialize vector (process 0 only)


  // Start Timer
    start = MPI_Wtime();


  }   // end 'if rank==0'

  // Calc dot product of subvector and send result
  if (rank == 0) {
    double sub_result;
    long ops;
    result = norm(vector, chunk_size);
    free(vector);
    for (i = 1; i < n_proc; i++) {
      MPI_Recv(&sub_result, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD, &status);
      result += sub_result;
    }
    //Stop Timer
    end = MPI_Wtime();
    ops = ((long)VectorLength) * 2;
    printf("Dot product = %f\n", result);
    printf("%f seconds elapsed.\n", end-start);
    printf("%ld operations completed.\n", ops);
    printf("%e seconds/operation.\n", (end-start)/((double)ops));

  } else {
    // Process subvector.
    MPI_Probe(0, tag, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_DOUBLE, &msg_len);
    //printf("Process %d received vector of len %d.\n", rank, msg_len);
    if (NULL == (vector = (double*) malloc(sizeof(double) * msg_len))) {
      printf("malloc failed on process %d...", rank);
    };
    MPI_Recv(vector, msg_len, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &status);
    result = norm(vector, msg_len);
    MPI_Send(&result, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);
    free(vector);
  }

  MPI_Finalize();

  exit(0);
  */
}
