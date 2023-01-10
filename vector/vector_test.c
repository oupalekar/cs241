/**
 * vector
 * CS 241 - Spring 2022
 */
#include "vector.h"
#include "stdio.h"
#include "signal.h"
#include "unistd.h"
#include "execinfo.h"
#include "callbacks.h"

void printvector(vector *v) {
    for(size_t i = 0; i < vector_size(v); i++) {
        printf("%d ", *(int*)vector_get(v,i));
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    // signal(SIGSEGV, handler);
    // Write your test cases here
    struct vector *v;
    struct vector *z;
    //Setup
    v = int_vector_create();
    //vector_create(int_copy_constructor, int_destructor, int_default_constructor);
    z = int_vector_create(); //vector_create(NULL, NULL, NULL);
    for(int i = 0; i < 10; i ++) {
        int x = i;
        vector_push_back(v, &x);
    }
    // printf("%d\n", *(int*)vector_at(v, 0));

    printvector(v);



    //Test Begin
    printf("%s\n", "======Begin=====");
    //Correct case
    void ** p = vector_begin(v);
    int* d = p[0];
    if(*d == 0) {
        printf("Test_begin_1 passed.\n");
    }
    //Test End
    printf("%s\n", "======End=====");
    //Correct case
    p = vector_end(v);
    d = *p;
    if(*d == 9) {
        printf("Test_end_1 passed.\n");
    }
    //Test Size
    printf("%s\n", "======Size=====");
    //Correct case
    size_t size = vector_size(v);
    if(size == 10) {
        printf("Test_size_1 passed.\n");
    }
    //Zero case:
    size = vector_size(z);
    if(size == 0) {
        printf("Test_size_2 passed.\n"); 
    }

    printf("%s\n", "======At=====");
    // printf("%d\n", (int)*vector_at(v, 3));
    if((int)*vector_at(v, 3) == 3) {
        printf("Test_at_1 passed.\n");
    }

    printf("%s\n", "======Set=====");
    int a = 4583945;
    void * s = &a;
    vector_set(v, 4, s);
    if((int)*vector_at(v,4) == 4583945) {
        printf("Test_set_1 passed.\n");
    }

    printvector(v);
    
    printf("%s\n", "======Get=====");
    if(*(int*)vector_get(v, 4) == 4583945) {
        printf("Test_get_1 passed.\n");
    }

    printf("%s\n", "======Front=====");
    // printf("%d\n", *(int*)(vector_front(v)[0]));
    if(*(int*)vector_front(v)[0] == 0) {
        printf("Test_front_1 passed.\n");
    }

    printf("%s\n", "======Back=====");
    // printf("%d\n", *(int*)(vector_front(v)[0]));
    if(*(int*)vector_back(v)[0] == 9) {
        printf("Test_back_1 passed.\n");
    }

    printf("%s\n", "======PopBack=====");
    // printf("%d\n", *(int*)(vector_front(v)[0]));
    vector_pop_back(v);
    if(*(int*)vector_back(v)[0] == 8 && vector_size(v) == 9) {
        printf("Test_popback_1 passed.\n");
    }
    printvector(v);

    printf("%s\n", "======Insert=====");
    // printf("%d\n", vector_size(v) == 10);
    a = 43454;
    s = &a;
    int b = 4354;
    int *t = &b;
    vector_insert(v, 8, s);
    vector_insert(v, 2, t);
    printf("%d\n", *(int*)vector_get(v,2));
    printf("%zu\n", vector_size(v));
    if(*(int*)vector_get(v,2) == 4354 && vector_size(v) == 10) {
        printf("Test_Insert_1 passed.\n");
    }
    printvector(v);

    printf("%s\n", "======Erase=====");
    // printf("%d\n", vector_size(v) == 10);
    vector_erase(v,8);
    printf("%zu\n", vector_size(v));
    // if(*(int*)vector_get(v,8) == 8 && vector_size(v) == 9) {
    //     printf("Test_Erase_1 passed.\n");
    // }
    printvector(v);

    // Test Resize
    printf("%s\n", "======Resize=====");
    vector_resize(v, 20);
    if(vector_size(v) == 20) {
        printf("Test_resize_1 passed. \n");
        int m = 3, *f = &m;
        vector_insert(v,15, f);
    }
    printvector(v);
    vector_resize(v, 15);
    if(vector_size(v) == 15) {
        printf("Test_resize_2 passed. \n");
    }
    printvector(v);
    vector_resize(v, 0);
    if(vector_size(v) == 0) {
        printf("Test_resize_3 passed. \n");
    }
    printvector(v);

    printf("%s\n", "======Capacity=====");
    size_t capacity = vector_capacity(v);
    if(capacity == 32) {
       printf("Test_capacity_1 passed. \n"); 
    }
    if(vector_capacity(z) == 8) {
       printf("Test_capacity_2 passed. \n"); 
    }
    printf("%s\n", "======Empty=====");
    vector_resize(v,10);
    if(!(vector_empty(v))) {
        printf("Test_empty_1 passed. \n");
    }
    if(vector_empty(z)) {
        printf("Test_empty_2 passed. \n");
    }
    
    printvector(v);

   printf("%s\n", "======Reserve====="); 
   vector_reserve(z, 9);
   if(vector_capacity(z) == 16) {
       printf("Test_reserve_1 passed.\n");
       if(vector_size(z) == 0) {
           printf("Test_reserve_size passed.\n");
       }
   }


    vector_destroy(v);

    return 0;
}
