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

struct dot_work_t **create_vector (int argc, char **argv) {

}

int dot_product_result (int sz, dot_result_t *res) {

}

dot_result_t *compute_dot (dot_work_t *work) {

}

int main(int argc, char **argv) {
  const unsigned int VectorLength = VECTOR_LENGTH; // in case VECTOR_LENGTH is expression
  int n_proc, rank, i, chunk_size, msg_len, tag=1;
  MPI_Status status;
  double *vector;
  double result, start, end;

  MPI_Init(&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &n_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  // Initialize vector (process 0 only)
  if (rank == 0) {
    if (NULL == (vector = (double*)malloc(sizeof(double) * VectorLength))) {
      printf ("malloc failed on process 0...");
    };

    srand(time(NULL));
    for (i = 0; i < VectorLength; i++) {
      vector[i] = (double)rand() / RAND_MAX;
    }

    printf("Generated vector of length %d.\n", i);
    printVector(vector, VectorLength, rank);

  /*** Start timer  ***/
    start = MPI_Wtime();

  // Chop up vector and send chunks to processes (0 only).
    chunk_size = VectorLength / n_proc;

    for (i = 1; i < n_proc; i++) {
      msg_len = chunk_size;
      if (i == n_proc-1) {
        msg_len += VectorLength % n_proc;
      }
      //printf("outgoing message len: %d\n", msg_len);
      MPI_Send(&vector[i*chunk_size], msg_len, MPI_DOUBLE,
            i, tag, MPI_COMM_WORLD);
    }
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
    /*** Stop timer ***/
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
}
