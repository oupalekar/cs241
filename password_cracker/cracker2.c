/**
 * password_cracker
 * CS 241 - Spring 2022
 */
#define _POSIX_C_SOURCE 200112L
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

typedef struct hacker_t {
    pthread_t thread;
    size_t index;
    size_t hashes;
} hacker;

static pthread_barrier_t wall_of_china;
static pthread_barrier_t berlin_wall;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char * pw;
static char * username;
static char * password;
static char * hash;
static int working;
static size_t thread_count_t;
void*  hack_password(void *);



int start(size_t thread_count) {
    thread_count_t = thread_count;
    pthread_barrier_init(&wall_of_china, NULL, thread_count + 1);
    pthread_barrier_init(&berlin_wall, NULL, thread_count + 1);
    
    hacker * hackers = malloc(thread_count * sizeof(hacker));

    for(size_t i = 0; i < thread_count; i++) {
        hackers[i].index = i + 1;
        hackers[i].hashes = 0;
        hackers[i].thread = 0;
        pthread_create(&(hackers[i].thread), NULL, hack_password, hackers + i);
    }

    char * line = NULL;
    char * line_copy = NULL;
    size_t size = 0;

    while(getline(&line_copy, &size, stdin) != -1) {
        line = line_copy;
        pw = NULL;
        working = 1;
        username = strsep(&line, " ");
        hash = strsep(&line, " ");
        password = line;
        password[strlen(password) - 1] = '\0';
        hash = hash;
        pthread_mutex_lock(&mutex);
        v2_print_start_user(username);
        pthread_mutex_unlock(&mutex);

        pthread_barrier_wait(&wall_of_china);
        double totalCPUTime = getCPUTime();
        double elapsedTime = getTime();
        pthread_barrier_wait(&berlin_wall);
        int hashCount = 0;
        for(size_t i = 0; i < thread_count; i++) {
            hashCount += hackers[i].hashes;
        }
        int result = (pw == NULL) ? 1 : 0;
        pthread_mutex_lock(&mutex);
        v2_print_summary(username, pw, hashCount, getTime() - elapsedTime, getCPUTime() - totalCPUTime, result);
        pthread_mutex_unlock(&mutex); 
        free(pw);
    }
    working = 0;
    pthread_barrier_wait(&wall_of_china);
    for(size_t i = 0; i < thread_count; i++) {
        pthread_join(hackers[i].thread, NULL);
    }
    pthread_barrier_destroy(&wall_of_china);
    pthread_barrier_destroy(&berlin_wall);
    free(hackers);
    free(line_copy);
    
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}



void * hack_password(void * h) {
    hacker *hack = h;
    while (1) {
        pthread_barrier_wait(&wall_of_china);
        if (working == 0) {
            break;
        }
        int prefix_len = getPrefixLength(password);
        char *start_pw = strdup(password);
        long start_index, count = 0;
        getSubrange(strlen(password) - prefix_len, thread_count_t, hack->index, &start_index, &count);
        setStringPosition(start_pw + prefix_len, start_index);
        pthread_mutex_lock(&mutex);
        v2_print_thread_start(hack->index, username, start_index, start_pw);
        pthread_mutex_unlock(&mutex);
        struct crypt_data cdata;
        cdata.initialized = 0;
        int status = 2;
        for (long i = 0; i < count; ++i) {
            if (pw != NULL) {
                status = 1;
                break;
            }
            hack->hashes++;
            if (strcmp(crypt_r(start_pw, "xx", &cdata), hash) == 0) {
                status = 0;
                pw = start_pw;
                break;
            }
            incrementString(start_pw);
        }
        if (status == 1 || status == 2) {
            free(start_pw);
            start_pw = NULL;
        }
        pthread_mutex_lock(&mutex);
        v2_print_thread_result(hack->index, hack->hashes, status);
        pthread_mutex_unlock(&mutex);
        pthread_barrier_wait(&berlin_wall);
        // if(start_pw != NULL) {free(start_pw);}
    }
    return NULL;
}
