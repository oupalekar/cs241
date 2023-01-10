/**
 * mini_memcheck
 * CS 241 - Spring 2022
 */
#include <stdio.h>
#include <stdlib.h>
// #include "mini_memcheck.h"

int main(int argc, char *argv[]) {
    // Your tests here using malloc and free
    char *x = malloc(10 * sizeof(char*));
    char *y = calloc(10, sizeof(char));
    char *z = realloc(x, 5 * sizeof(char));
    char *e = realloc(NULL, 10);
    free(y);
    free(z);
    free(e);
    return 0;
}