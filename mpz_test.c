#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

#define TEST_NUM  "123456789"

int serialize_mpz(mpz_t bignum, unsigned char **dest, int *length) {
  unsigned char *int_count = *dest;
  unsigned char *temp_mpz;
  unsigned int mpz_size;
  temp_mpz = mpz_export(NULL, &mpz_size, 1, 1, 0, bignum);
  printf(mpz_size);
  *dest = temp_mpz;
  *length = mpz_size;
  return 0;
}

int main(int argc, char **argv) {
  mpz_t test;
  unsigned char *target;
  int length, i;
  test = mpz_init_set_str(test, TEST_NUM, 10);
  serialize_mpz(test, &target, &length);
  printf("n_bytes: %d\n", length);
  for(i=0;i<length;i++) {
    printf("%X\n", *target++);
  }
  free(target);

  return 0;
}
