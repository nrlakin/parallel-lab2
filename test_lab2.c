#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct dubstruct {
  double number;
};

int main(int argc, char** argv) {
    struct dubstruct test;
    double test2[5] = {1.0,1.1,1.2,1.3,1.4};
    double* dubptr = test2;
    test.number = 3.14159;
    printf("Double: %ld\n", sizeof(double));
    printf("Dubstruct: %ld\n", sizeof(struct dubstruct));
    printf("Dubstruct address (explicit): %X\n", (unsigned int)(&test));
    printf("Dubarray address (implicit): %X\n", (unsigned int)(test2));
    printf("Dubarray address (explicit): %X\n", (unsigned int)(&test2));
    printf("Double address: %X\n", (unsigned int)(&(test.number)));
    printf("DubPtr address: %X\n", (unsigned int)(dubptr++));
    printf("New DubPtr address: %X\n", (unsigned int)(dubptr));
    return 0;
}
