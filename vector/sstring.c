/**
 * vector
 * CS 241 - Spring 2022
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>
#include <ctype.h>

char * strtoke(char *str, const char *delim);

struct sstring {
    // Anything you want
    char * string;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring  * str = malloc(sizeof(sstring));
    char * temp = malloc(strlen(input) + 1);
    strcpy(temp, input);
    str->string = temp;
    return str;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char* str = malloc(strlen(input->string) + 1);
    strcpy(str, input->string);
    return str;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    this->string = realloc(this->string, strlen(this->string) + strlen(addition->string) + 1);
    strcat(this->string, addition->string);
    return strlen(this->string);
}

vector *sstring_split(sstring *this, char delimiter) {
    vector *v = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    // char
    
    char str[strlen(this->string)];
    strcpy(str, this->string);
    char * word;
    for(word = strtoke(str, &delimiter); word; word = strtoke(NULL, &delimiter)) {
        // printf("%s\n", word);
        // if(word == NULL) vector_push_back(v, "\0");
        //else
        vector_push_back(v, word);
    }
    return v;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    if(offset > strlen(this->string)){ return -1;}
    
    char * temp = this->string + offset;
    
    while(temp) {
       if(strncmp(temp, target, strlen(target)) == 0) {
           break;
       }
       temp++;
       
    }
    int diff = strlen(substitution) - strlen(target);
    printf("%s\n", temp);
    if(temp) {
        printf("reached\n");
        char * new_str = malloc(strlen(this->string) + diff + 1);
        strncpy(new_str, this->string, strlen(this->string) - strlen(temp));
        // printf("%s\n", new_str);
        strcpy(new_str + strlen(this->string) - strlen(temp), substitution);
        // printf("%s\n", new_str);
        strcpy(new_str + strlen(this->string) - strlen(temp) + strlen(substitution), temp + strlen(target));
        printf("%s\n", new_str);
        free(this->string);
        this->string = new_str; 
        return 0;
    }
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    char * temp = this->string;
    int i = 0;
    while(i < start) {
        temp++;
        i++;
    }
    size_t diff = end - start;
    printf("%s\n", temp);
    char to_ret[diff + 1];
    strncpy(to_ret, temp, diff);
    to_ret[diff] = '\0';
    printf("%s\n", to_ret);
    char * r  = to_ret;
    printf("%s\n", r);
    // free(to_ret);
    return strdup(r);

}

void sstring_destroy(sstring *this) {
    // your code goes here
    free(this->string);
    free(this);
}

/**
 * @brief Code adapted from https://stackoverflow.com/questions/42315585/split-string-into-tokens-in-c-when-there-are-2-delimiters-in-a-row 
 * 
 * @param str 
 * @param delim 
 * @return char* 
 */
char* strtoke(char *str, const char *delim)
{
  static char *start = NULL; /* stores string str for consecutive calls */
  char *token = NULL; /* found token */
  /* assign new start in case */
  if (str) start = str;
  /* check whether text to parse left */
  if (!start) return NULL;
  /* remember current start as found token */
  token = start;
  /* find next occurrence of delim */
  start = strpbrk(start, delim);
  /* replace delim with terminator and move start to follower */
  if (start) *start++ = '\0';
  /* done */
  return token;
}