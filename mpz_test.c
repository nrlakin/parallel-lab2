#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#define TEST_NUM  "123456789123456789123456789123456789"

/*
int serialize_jobs(struct userdef_job_t **start_job, int n_jobs, unsigned char **array, int *len) {
  unsigned char *destPtr = *array;
  unsigned char *temp_mpz;
  struct userdef_job_t **job = start_job;
  size_t mpz_size;
  int length, i;
  for(i=0; i<n_jobs; i++) {
    if(*job == NULL) break;


  }
  temp_mpz = mpz_export(NULL, &mpz_size, 1, 1, 1, 0, bignum);
  len = mpz_size;
  *dest = (unsigned char*) malloc(sizeof(int) + (sizeof(unsigned char) * len)
                    + 2*sizeof(unsigned long));
  memcpy(destPtr, &len, sizeof(int));
  destPtr += 4;
  memcpy(destPtr, temp_mpz, len);
  destPtr += len * sizeof(unsigned char);

  free(temp_mpz);
  return 0;
}*/

int main(int argc, char **argv) {
  mpz_t test;
  unsigned char *target;
  int length, i;
  mpz_init_set_str(test, TEST_NUM, 10);
  //serialize_mpz(test, &target, &length);
  //printf("n_bytes: %d\n", length);
  length = (mpz_sizeinbase(test, 2) + 7)/8;
  printf("test_count: %d\n", length);
  //for(i=0;i<length;i++) {
  //  printf("%X\n", target[i]);
  //}
  mpz_clear(test);
  //free(target);
  return 0;
}
