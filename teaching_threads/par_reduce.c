/**
 * teaching_threads
 * CS 241 - Spring 2022
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct data_t {
    int *list;
    size_t list_len;
    reducer reduce_func;
    int base_case;
} data_t;
/* You should create a start routine for your threads. */
size_t * items_per_thread(size_t, size_t);
void * start_routine(void * ptr);

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    if(num_threads > list_len) {
        num_threads = list_len;
    }
    pthread_t * threads = calloc(num_threads, sizeof(pthread_t));
    data_t * data = calloc(num_threads, sizeof(data_t));
    int * retvals = calloc(num_threads, sizeof(int));
    size_t* starting_pts = items_per_thread(list_len, num_threads);
    size_t sum = 0;

    int running_threads = 0; 

    for(size_t i = 0; i < num_threads; i++) {
        if(starting_pts[i] > 0) {
            data[i] = (data_t){list + sum, starting_pts[i], reduce_func, base_case};
            pthread_create(threads + i, NULL, start_routine, data + i);
            sum += starting_pts[i];
            running_threads++;
        }
    }
    int i = 0;
    while(i < running_threads) {
        void * temp = NULL;
        pthread_join(threads[i], &temp);
        retvals[i] = *(int*)temp;
        free(temp);
        i++;
    }
    int ret = reduce(retvals, num_threads, reduce_func, base_case);
    free(threads);
    free(data);
    free(starting_pts);
    free(retvals);
    /* Your implementation goes here */
    return ret;
}


size_t * items_per_thread(size_t list_len, size_t num_threads) {
    size_t* ret = calloc(num_threads, sizeof(size_t));

    size_t x = list_len/num_threads;
    size_t r = list_len % num_threads;

    for(size_t i = 0; i < num_threads; i++) {
        ret[i] = x;
    }

    for(size_t i = 0; i < r; i++) {
        ret[i] += 1;
    }
    return ret;
}



void * start_routine(void * ptr) {
    int * ret = calloc(1, sizeof(int));
    data_t* data = (data_t*)ptr;
    *ret = reduce(data->list, data->list_len, data->reduce_func, data->base_case);
    return ret;
}