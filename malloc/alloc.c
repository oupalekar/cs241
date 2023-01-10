/**
 * malloc
 * CS 241 - Spring 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

typedef struct _metadata_entry_t {
    void * ptr;
    size_t size;
    int free;
    struct _metadata_entry_t *next;
    struct _metadata_entry_t *prev;
} metadata_t;


static size_t sbrk_height = 0;
static size_t alloc_mem = 0;
static metadata_t * head = NULL;

char splitMem(size_t, metadata_t*);
void combineBlock(metadata_t*);
void combinePrev(metadata_t*);
void combineNext(metadata_t*);
void* setBlock(metadata_t * chosen, size_t size);
/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    void *ptr = malloc(num * size);
    if(ptr != NULL) {
        memset(ptr, 0, num*size);
    }
    return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    if(size <= 0) return NULL;

    metadata_t * temp = head;
    metadata_t * chosen = NULL;

    if(sbrk_height - alloc_mem >= size) { // If there is already space in the "heap". If not assumes early malloc call 
        while(temp != NULL) {
            if(temp->free && temp->size >= size) {
                chosen = temp;
                if(splitMem(size, temp) == 1) {
                    alloc_mem += sizeof(metadata_t); //added extra metadata
                }
                break;
            }
            temp = temp->next;
        }
    }
    return setBlock(chosen, size);
    // return chosen->ptr;
}

void* setBlock(metadata_t * chosen, size_t size) {
    if(chosen != NULL) {
        chosen->free = 0;
        alloc_mem += chosen->size;
    } else if (head != NULL && head->free == 1) {
        int size_diff = size - head->size;
        if(sbrk(size_diff) == (void*) -1) {
            return NULL;
        }
        head->size = size;
        head->free = 0;
        chosen = head;
        sbrk_height += size_diff;
        alloc_mem += head->size;
    } else { //Head is either null or not free
        chosen = sbrk(sizeof(metadata_t) + size);
        if(chosen == (void*) -1) {
            return NULL;
        }
        chosen->ptr = chosen + 1;
        chosen->free = 0;
        chosen->size = size;
        chosen->next = head;
        if(head != NULL) {
            chosen->prev = head->prev;
            head->prev = chosen;
        } else {chosen->prev = NULL;}
        head = chosen;
        sbrk_height += sizeof(metadata_t) + size;
        alloc_mem += sizeof(metadata_t) + size;
    }
    return chosen->ptr;
}


void free(void* ptr) {
    if(!ptr) return;
    metadata_t* temp = ptr - sizeof(metadata_t);
    assert(temp->free == 0);
    // if(temp->free == 0) {
    temp->free = 1;
    alloc_mem -= temp->size;
    combineBlock(temp);
    // }
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    if(ptr == NULL) {
        return malloc(size);
    }

    metadata_t *realloc_data = ((metadata_t *)ptr) - 1;
    assert(realloc_data->ptr == ptr);
    assert(realloc_data->free == 0);
    if(size == 0) {
        free(ptr);
        return NULL;
    }
    if(splitMem(size, realloc_data)) {alloc_mem -= realloc_data->prev->size;}
    if(realloc_data->size >= size) {
        return ptr;
    } else if(realloc_data->prev && realloc_data->prev->free && realloc_data->prev->size + realloc_data->size + sizeof(metadata_t) >= size) {
       
        combinePrev(realloc_data);
        return realloc_data->ptr;  
    }
    void *to_ret = malloc(size);
    memcpy(to_ret, ptr, realloc_data->size);
    free(ptr);
    return to_ret;
}


char splitMem(size_t size, metadata_t* block) {
    if(block->size >= 2*size){
        if(block->size - size >=1024) {
            metadata_t * block_2 = block->ptr + size; // Skip to next meta_data
            
            block_2->ptr = block_2 + 1;
            block_2->size = block->size - size - sizeof(metadata_t);
            block_2->free = 1;
            block_2->next = block;
            
            if(block->prev) {
                block->prev->next = block_2;
            } else {
                head = block_2;
            }
            block_2->prev = block->prev;
            block->size = size;
            block->prev = block_2;
            return 1;
        }
        return 0;
    }
    return 0;
}

void combineBlock(metadata_t* ptr) {
    if(ptr->prev && ptr->prev->free == 1) {
        combinePrev(ptr);
        alloc_mem -= sizeof(metadata_t);
    }
    if(ptr->next && ptr->next->free == 1) {
        combineNext(ptr);
        alloc_mem -= sizeof(metadata_t); 
    }
}

void combinePrev(metadata_t* ptr) {
    ptr->size += sizeof(metadata_t) + ptr->prev->size;
    ptr->prev = ptr->prev->prev;
    if(ptr->prev == NULL) {
        head = ptr;
    } else {
        ptr->prev->next = ptr;
    }
        
}
void combineNext(metadata_t* ptr){
    ptr->next->size += sizeof(metadata_t) + ptr->size;
    ptr->next->prev = ptr->prev;
    if(ptr->prev == NULL) {
        head = ptr->next;
    } else {
        ptr->prev->next = ptr->next;
    }
}