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
#include "mw_api.h"

// Must be less than max int!
#define VECTOR_LENGTH 1000000000
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
void printVector(double * vec, int len, int rank) {
  int i;
  if(len > 10) return;
  for (i=0; i<len; i++) {
    printf("%d,%d: %f\n", rank, i, *vec++);
  }
}

struct dot_work_t {
  int length;
  double *vector;
};

struct dot_result_t {
  double product;
};

struct dot_work_t **create_jobs (int argc, char **argv) {
  int i, chunk_size;
  const unsigned int VectorLength = VECTOR_LENGTH; // in case VECTOR_LENGTH is expression
  double *vector;
  struct dot_work_t **job_queue;
  struct dot_work_t *jobs;

  if (NULL == (vector = (double*)malloc(sizeof(double) * VectorLength))) {
    printf ("malloc failed on allocating vector...");
    return NULL;
  };
  if (NULL == (jobs = (struct dot_work_t*)malloc(sizeof(struct dot_work_t) * N_JOBS))) {
    printf ("malloc failed on allocating jobs...");
    return NULL;
  };
  if (NULL == (job_queue = (struct dot_work_t**)malloc(sizeof(struct dot_work_t*) * N_JOBS + 1))) {
    printf ("malloc failed on allocating job queue...");
    return NULL;
  };

  srand(time(NULL));
  for (i = 0; i < VectorLength; i++) {
    vector[i] = (double)rand() / RAND_MAX;
  }

  printf("Generated vector of length %d.\n", i);

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

int dot_product_result (int sz, struct dot_result_t *res) {
  double dot_product = 0;
  for(i=0; i<sz; i++) {
    if (res == NULL)return 0;     // early termination
    dot_product += res->product;
    res++;
  }
  if (res != NULL)return 0;
  printf("Dot product: %d", dot_product);
  return 1;
}

struct dot_result_t *compute_dot (struct dot_work_t *work) {
  struct dot_result_t *result;
  int i;
  if (NULL == (result = (struct dot_result_t*)malloc(sizeof(struct dot_result_t))) {
    printf ("malloc failed on compute_dot...");
    return NULL;
  };
  result->product = norm(work->vector, work->length);
  return result;
}

int main(int argc, char **argv) {
  struct mw_api_spec f;

  MPI_Init(&argc, &argv);

  f.create = create_jobs;
  f.result = dot_product_result;
  f.compute = compute_dot;
  f.work_sz = sizeof(struct dot_work_t);
  f.res_sz = sizeof(struct dot_result_t);

  MW_Run(argc, argv, &f);

  MPI_Finalize();
  //struct dot_work_t **queue = create_jobs(argc, argv);
  //free(queue[0]->vector);
  //free(queue[0]);
  //free(queue);
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
