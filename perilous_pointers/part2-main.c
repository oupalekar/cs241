/**
 * perilous_pointers
 * CS 241 - Spring 2022
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int x = 132;
    int *ptr = &x;
    second_step(ptr);

    x = 8942;
    int **twoptr = &ptr;
    double_step(twoptr);

    char arr[] = {1,2,3,4,5,15,0,0,0};
    strange_step(arr);

    char empty[] = {1,2,3,0,'\0'};
    empty_step((void*)empty);
    // free(value);


    char v[] = {'a','a','a','u', '\0'};
    two_step((void*)v, v);

    char * first = "oooo";
    char * second = "oo";
    char * third = second +2;
    three_step(first, second, third);

    first = "1234567890";
    second = "o1:3243"; 
    third = "oooBo";
    step_step_step(first, second, third);


    char * odd = "A";
    int b = 65;
    it_may_be_odd(odd, b);

    char tok[10] = "ha,CS241\0";
    tok_step(tok);
 
    // char * df = 0x0F;
    // printf("%d", df[0] == 1);
    char arx[] = {1,2,3,'\0'};
    the_end((void*)arx, (void*)arx);
    return 0;
}
