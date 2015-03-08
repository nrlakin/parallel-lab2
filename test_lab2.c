#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gmp.h>


#define START_NUM "10000000"
#define N_JOBS  10

struct dubstruct {
  double number;
};

int main(int argc, char** argv) {
    mpz_t n, root, chunk_size;

    mpz_init_set_str(n, START_NUM, 10);
    mpz_init(root);
    mpz_init(chunk_size)
    mpz_sqrt(root, n);
    mpz_tdiv_q_ui(chunk_size, n, N_JOBS);

    mpz_out_str(stdout,10,n);
    printf ("\n");
    mpz_out_str(stdout,10, root);
    printf ("\n");
    mpz_out_str(stdout,10,chunk_size);
    printf ("\n");

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
    return 0;
}
