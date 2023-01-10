/**
 * critical_concurrency
 * CS 241 - Spring 2022
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue *q = malloc(sizeof(queue));
    if(!q) return NULL;
    q->max_size = max_size;
    q->size = 0;
    pthread_cond_init(&q->cv, NULL);
    pthread_mutex_init(&q->m, NULL);
    q->head = NULL;
    q->tail = NULL;
    return q;
}

void queue_destroy(queue *this) {
    if(!this) return;
    queue_node * curr = this->head;
    while(curr != NULL) {
        queue_node * q = curr;
        curr = curr->next;
        free(q);
    }

    pthread_cond_destroy(&this->cv);
    pthread_mutex_destroy(&this->m);
    free(this);
    /* Your code here */
}

void queue_push(queue *this, void *data) {
    pthread_mutex_lock(&this->m);
    while(this->size >= this->max_size) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    queue_node *q = malloc(sizeof(queue_node));
    q->data = data;
    q->next = NULL;
    
    if(this->size == 0) {
        this->head = q;
        this->tail = q;
    } else {
        this->tail->next = q;
        this->tail = q;
    }
    this->size++;
    pthread_cond_broadcast(&this->cv);
    pthread_mutex_lock(&this->m);
    /* Your code here */
}

void *queue_pull(queue *this) {
    pthread_mutex_lock(&this->m);
    while(this->size <= 0) pthread_cond_wait(&this->cv, &this->m);

    queue_node * q = this->head;
    if(this->head == this->tail) {
        this->head = NULL;
        this->tail = NULL;
    } else {
        this->head = q->next;
    }
    void * data = q->data;
    free(data);
    this->size--;
    pthread_cond_broadcast(&this->cv);
    pthread_mutex_lock(&this->m);
    return data;
}
