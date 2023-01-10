/**
 * password_cracker
 * CS 241 - Spring 2022
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "crypt.h"
#include "math.h"


typedef struct hacker_t {
    pthread_t thread;
    size_t id;
} hacker;


static queue * q = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int numRecovered;
static hacker * hackers;

queue* fill_queue();
void * hack_password(void*);
void copy_string(char*, char**, char**, char**);

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    q = fill_queue();

    hackers = malloc(sizeof(hacker) * thread_count);

    for(size_t i = 0; i < thread_count; i++) {
        hackers[i].id = i + 1;
        pthread_create(&(hackers[i].thread), NULL, hack_password, hackers + i);
    }

    for(size_t i = 0; i < thread_count; i++) {
        pthread_join(hackers[i].thread, NULL);
    }
    free(hackers);
    queue_destroy(q);


    // pthread_t * threads = malloc(sizeof(pthread_t) * thread_count);



    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}


queue * fill_queue() {
    queue * q = queue_create(100);
    char input[1024];
    while (1) {
        if (fgets(input, 1024, stdin) == NULL) {
            break;
        } else {
        /* Here we suppose the fgets() has reached a '\n' character... */
            for (char* s = input; (*s != '\n') && isspace(*s); s++)
                ; /* skipping blanks */
            if (feof(stdin))
                break; /* Blank line */
            else {
                queue_push(q, strdup(input + '\0'));
            }
        }
    }
    queue_push(q, NULL);
    return q;
}


void * hack_password(void * h) {
    hacker * hacker = h;
    
    char * task = NULL;
    char * temp = NULL;
    while((temp = task = queue_pull(q)) != NULL) {
        char * username = strsep(&task, " ");
        char * hash = strsep(&task, " ");
        char *password = task;
        password[strlen(password) - 1] = '\0';
        
        
        v1_print_thread_start(hacker->id, username);
        double start_time = getCPUTime();
        // double end_time = 0;
        int prefix = getPrefixLength(password);
        int result = 1;
        int hash_count = 0;
        if(prefix == (int)strlen(password)) {
            result = 0;
        } else {
            int suffix = strlen(password) - prefix;
            setStringPosition(password + prefix, 0);
            struct crypt_data cdata;
            cdata.initialized = 0;
            double range = pow(26, suffix);
            // printf("%s\n", password);
            for(int i = 0; i < range; i++) {
                ++hash_count;
                const char * hashed = crypt_r(password, "xx",&cdata);
                // printf("%s\n", password);
                if(strcmp(hashed, hash) == 0) {
                    
                    result = 0;
                    // end_time = getCPUTime();
                    pthread_mutex_lock(&mutex);
                    ++numRecovered;
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                incrementString(password);
            }
        }
        v1_print_thread_result(hacker->id, username, password, hash_count, getCPUTime() - start_time, result);
        free(temp);
        task = NULL;
        // printf("%s\n", task);
        // printf("%s\n", temp);
    }
    queue_push(q, NULL);
    // free(temp);
    return NULL;
}

void copy_string(char * task, char ** username, char ** hash, char ** password) {
    char * index = strchr(task, ' ');
    printf("here");
    strncpy(*username, task, index - task);
    *username[strlen(*username)] = '\0';
    task = index + 1;
    index = strchr(task, ' ');
    strncpy(*hash, task, index - task);
    task = index + 1;
    printf("%s\n", task);
    strncpy(*password, task, strlen(task));
    
}