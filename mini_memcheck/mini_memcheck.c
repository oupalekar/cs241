/**
 * mini_memcheck
 * CS 241 - Spring 2022
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

meta_data *head = NULL;
size_t total_memory_requested = 0;
size_t total_memory_freed = 0;
size_t invalid_addresses = 0;

int invalid_pointer(void *);

meta_data * get_previous_memory(meta_data*);

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    if(request_size == 0) {
        return NULL;
    }

    meta_data *memory = malloc(sizeof(meta_data) + request_size);
    if(!memory){return NULL;}
    
    memory->request_size = request_size;
    memory->filename = filename;
    memory->instruction = instruction;
    memory->next = NULL;

    if(head == NULL) {
        head = memory;
    } else {
        meta_data *temp = head;
        while(temp->next) {
            temp = temp->next;
        }
        temp->next = memory;
    }
    total_memory_requested += request_size;
    return (void*)(memory + 1);
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    if(num_elements == 0) {
        return NULL;
    }
    meta_data *memory = mini_malloc(num_elements * element_size, filename, instruction);
    memset(memory, 0, num_elements * element_size);
    return memory;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {

    if(payload == NULL) {
        return mini_malloc(request_size, filename, instruction);
    }

    if(request_size == 0) {
        mini_free(payload);
        return NULL;
    }
    // fprintf(stdout, "%p\n", payload);
    if(invalid_pointer(payload)) {
        // fprintf(stdout, "In Realloc");
        invalid_addresses++;
        return NULL;
    }

    meta_data* data = (meta_data*)payload - 1; 
    meta_data* prev = NULL;
    meta_data* curr = head;
    if(curr == NULL || data == head) {}
    else {
        while(curr->next != data) {
            curr = curr->next;
        }
        prev = curr;
    }

    meta_data* new_data = realloc(data, request_size + sizeof(meta_data));
    // fprintf(stdout, "%p\n", data);
    if(new_data == NULL) {return NULL;}

    if(new_data != data) {
        if(prev->next != NULL) {
            prev->next = new_data;
        } else {
            head = new_data;
        }
    }

    if(request_size > new_data->request_size) {
        total_memory_requested += request_size - new_data->request_size;
    } else {
        total_memory_freed += new_data->request_size - request_size;
    }
    new_data->request_size = request_size;
    return (void*)(new_data + 1);
}

void mini_free(void *payload) {
    if(!payload) {
        return;
    }

    if(invalid_pointer(payload)) {
        // fprintf(stdout, "In free");
        invalid_addresses++;
        return;
    }
    meta_data* data = (meta_data*)payload - 1;
    meta_data* prev = NULL;
    meta_data* curr = head;
    if(curr == NULL || data == head) {}
    else {
        while(curr->next != data) {
            curr = curr->next;
        }
        prev = curr;
    }
    if(prev != NULL) {
        prev->next = data->next;
    } else {
        head = data->next;
    }
    total_memory_freed += data->request_size;
    free(data);
}

int invalid_pointer(void * ptr){
    if(head == NULL) {
        return 1;
    }
    if(ptr == NULL) {
        return 0;
    } 
    meta_data *curr = head;
    while(curr) {
        if ((void*)(curr + 1) == ptr) {
            return 0;
        }
        // fprintf(stdout, "%p ", curr->next);
        curr = curr->next;
    }
    return 1;
}
meta_data * get_previous_memory(meta_data* md) {
    if(head == NULL || md == head) {
        return NULL;
    }
    meta_data * curr = head;
    while(curr->next != NULL) {
        if(curr->next == md) {
            return curr;
        }
        curr = curr->next;
    }
    return curr;
}