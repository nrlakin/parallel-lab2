#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>


#define START_NUM "101"
#define N_JOBS  3

struct userdef_work_t {
  mpz_t target;
  mpz_t rangeStart;
  mpz_t rangeEnd;
};

struct userdef_result_t {
  int length;
  mpz_t *factors;
};

struct dubstruct {
  double number;
};

int main(int argc, char** argv) {
    // mpz_init_set_str(n, START_NUM, 10);
    // mpz_init(root);
    // mpz_init(chunk_size)
    // mpz_sqrt(root, n);
    // mpz_tdiv_q_ui(chunk_size, n, N_JOBS);

    struct userdef_work_t **job_queue;
    struct userdef_work_t *jobs;

    if (NULL == (jobs = (struct userdef_work_t*)malloc(sizeof(struct userdef_work_t) * N_JOBS))) {
      printf ("malloc failed on allocating jobs...");
    };

    if (NULL == (job_queue = (struct userdef_work_t**)malloc(sizeof(struct userdef_work_t*) * N_JOBS + 1))) {
      printf ("malloc failed on allocating job queue...");
    };

    int i;
    mpz_t n, root, chunk_size;

    mpz_init_set_str(n, START_NUM, 10);
    mpz_init(root);
    mpz_init(chunk_size);
    mpz_sqrt(root, n);
    mpz_tdiv_q_ui(chunk_size, root, N_JOBS);

    mpz_out_str(stdout,10,n);
    printf ("\n");
    mpz_out_str(stdout,10, root);
    printf ("\n");
    mpz_out_str(stdout,10,chunk_size);
    printf ("\n");


    for (i = 0; i < N_JOBS; i++) {
      mpz_init_set(jobs[i].target, n);
      mpz_init(jobs[i].rangeStart);
      mpz_mul_si(jobs[i].rangeStart, chunk_size, i);
      mpz_init(jobs[i].rangeEnd);
      if (i == N_JOBS-1) {
        mpz_set(jobs[i].rangeEnd, root+1);
      } else {
        mpz_mul_si(jobs[i].rangeEnd, chunk_size, i+1);
      }
      mpz_out_str(stdout,10,jobs[i].target);
      printf ("\n");
      mpz_out_str(stdout,10, jobs[i].rangeStart);
      printf ("\n");
      mpz_out_str(stdout,10, jobs[i].rangeEnd);
      printf ("\n");
      job_queue[i] = &(jobs[i]);
    }
    job_queue[i] = NULL;
    // struct dubstruct test;
    // double test2[5] = {1.0,1.1,1.2,1.3,1.4};
    // double* dubptr = test2;
    // test.number = 3.14159;
    // printf("Double: %ld\n", sizeof(double));
    // printf("Dubstruct: %ld\n", sizeof(struct dubstruct));
    // printf("Dubstruct address (explicit): %X\n", (unsigned int)(&test));
    // printf("Dubarray address (implicit): %X\n", (unsigned int)(test2));
    // printf("Dubarray address (explicit): %X\n", (unsigned int)(&test2));
    // printf("Double address: %X\n", (unsigned int)(&(test.number)));
    // printf("DubPtr address: %X\n", (unsigned int)(dubptr++));
    // printf("New DubPtr address: %X\n", (unsigned int)(dubptr));
    free(job_queue[0]);
    free(job_queue);
    return 0;
}
