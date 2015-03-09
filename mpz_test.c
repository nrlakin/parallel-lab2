#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#define TEST_NUM  "123456789"
#define WORD_SIZE   1
#define ENDIAN      1
#define NAIL        0
#define ORDER       1

struct userdef_work_t {
  mpz_t target;
  unsigned long rangeStart;
  unsigned long rangeEnd;
};

int get_mpz_length(mpz_t bignum) {
  int numb = 8 * WORD_SIZE - NAIL;
  return (mpz_sizeinbase(bignum, 2) + numb-1)/numb;
}

int serialize_jobs(struct userdef_work_t **start_job, int n_jobs, unsigned char **array, int *len) {
  unsigned char *destPtr = *array;
  unsigned char *temp_mpz;
  struct userdef_work_t **job = start_job;
  size_t mpz_size;
  int i, mpz_len, length = 0;
  for(i=0; i<n_jobs; i++) {
    if(*job == NULL) break;
    length += sizeof(int);    //for mpz size
    length += get_mpz_length((*job)->target);
    length += 2*sizeof(unsigned long);  //for start/end
    job++;
  }
  if (NULL == (*array = (unsigned char*)malloc(sizeof(unsigned char) * length))) {
    printf("malloc failed on allocating send buffer\n");
    return 0;
  }
  job = start_job;
  *len = length;
  for(i=0; i<n_jobs; i++) {
    if(*job == NULL)break;
    mpz_len = get_mpz_length((*job)->target);
    memcpy(destPtr, &mpz_len, sizeof(int));
    destPtr += sizeof(int);
    mpz_export(destPtr, &mpz_size, ORDER, WORD_SIZE, ENDIAN, NAIL, (*job)->target);
    if (mpz_len != mpz_size) {
      printf("mpz error--different functions report different sizes.\n");
      return 0;
    }
    destPtr += mpz_len;
    memcpy(destPtr, &((*job)->rangeStart), sizeof(unsigned long));
    destPtr += sizeof(unsigned long);
    memcpy(destPtr, &((*job)->rangeEnd), sizeof(unsigned long));
    destPtr += sizeof(unsigned long);
    job++;
  }
  return 1;
}

int main(int argc, char **argv) {
  struct userdef_work_t test;
  struct userdef_work_t *work_queue[2];
  unsigned char *target;
  int length, i;
  mpz_init_set_str(test.target, TEST_NUM, 10);
  test.rangeStart = 1;
  test.rangeEnd = 5;
  work_queue[0] = &test;
  work_queue[1] = NULL;
  serialize_jobs(work_queue, 1, &target, &length);
  printf("serialized length: %d\n", length);
  for(i=0; i<length; i++) {
    printf("%X\n", target[i]);
  }
  //for(i=0;i<length;i++) {
  //  printf("%X\n", target[i]);
  //}
  mpz_clear(test.target);
  free(target);
  return 0;
}
