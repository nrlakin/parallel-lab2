#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#define TEST_NUM  "123456789123456789123456789123456789"

int serialize_mpz(mpz_t bignum, unsigned char **dest, int *length) {
  unsigned char *int_count = *dest;
  unsigned char *temp_mpz;
  size_t mpz_size;
  int len;
  temp_mpz = mpz_export(NULL, &mpz_size, 1, 1, 1, 0, bignum);
  len = mpz_size;
  printf("%d\n", (int)mpz_size);
  *dest = (unsigned char*) malloc(sizeof(unsigned char) * len +1);
  *length = len;
  memcpy(*dest, temp_mpz, len);
  free(temp_mpz);
  return 0;
}

int main(int argc, char **argv) {
  mpz_t test;
  unsigned char *target;
  int length, i;
  mpz_init_set_str(test, TEST_NUM, 10);
  serialize_mpz(test, &target, &length);
  printf("n_bytes: %d\n", length);
  for(i=0;i<length;i++) {
    printf("%X\n", *target[i]);
  }
  mpz_clear(test);
  free(target);
  return 0;
}
