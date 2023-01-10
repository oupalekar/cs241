/**
 * critical_concurrency
 * CS 241 - Spring 2022
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"
void test_one();
int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: %s test_number return_code\n", argv[0]);
        exit(1);
    }
    printf("Please write tests cases\n");
    if(argv[1] == 0) {
        test_one();
    }
    return 0;
}

void test_one() {
    // queue *q = queue_create(10);
}