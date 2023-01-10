/**
 * extreme_edge_cases
 * CS 241 - Spring 2022
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

char ** split_string(const char * input_str) {
    char ** output_str = NULL;
    int numSentences = 0;
    size_t start_char = 0;
    while(start_char < strlen(input_str)) {
        size_t end_char = start_char;
        for(; end_char < strlen(input_str); end_char++) {
            if(ispunct(input_str[end_char])) {
                // printf("%zu\n", end_char);
                char * str = malloc(end_char - start_char + 1);
                strncpy(str, input_str + start_char, end_char - start_char);
                str[end_char++ - start_char] = '\0';
                output_str = realloc(output_str, ++numSentences * sizeof(char *));
                output_str[numSentences - 1] = str;
                break;
            }
        }
        start_char = end_char;
    }
    output_str = realloc(output_str, (numSentences + 1) * sizeof(char *));
    output_str[numSentences] = NULL;
    return output_str;
}

// void rmwhitespace(char ** input_str) {
//     char * str = malloc(strlen(*input_str) + 1);
//     size_t index = 0;
//     for(size_t i = 0; i < strlen(*input_str); i++) {
//         if(!isspace((*input_str)[i])) {
//             str[index++] = (*input_str)[i];
//         }
//     }
//     str[index] = '\0';
//     free(*input_str);
//     *input_str = str;
// }

void rmwhitespace(char * s){
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++));
}

void camelCase(char ** input_str) {
    while(*input_str) {
        int first_word = 0;
        int startOfWord = 0;
        for(size_t index = 0; index < strlen(*input_str); index++) {
           if(!isspace((*input_str)[index]) && !first_word) {
               first_word = 1;
               if(isalpha((*input_str)[index])) {
                   if((*input_str)[index] >= 'A' && (*input_str)[index] <= 'Z') {
                       (*input_str)[index] += 32;
                   }
               }
           }
            if(isspace((*input_str)[index]) && first_word) {
                startOfWord = 1;
            }
            if(isalpha((*input_str)[index]) && !startOfWord) {
                if((*input_str)[index] >= 'A' && (*input_str)[index] <= 'Z') {
                       (*input_str)[index] += 32;
                }
            } 
            if(isalpha((*input_str)[index]) && startOfWord) {
                startOfWord = 0;
                if((*input_str)[index] >= 'a' && (*input_str)[index] <= 'z') {
                    (*input_str)[index] -= 32;
                } 
            }
        }
       input_str++;
    }
}


char **camel_caser(const char *input_str) {
    if(input_str == NULL) {return NULL;}
    char ** output_str = split_string(input_str);
    char ** temp = output_str;
    camelCase(output_str);
    while(*temp) {
        rmwhitespace(*temp);
        // printf("%s\n", *temp);
        temp++;
    }
    
    return output_str;
}

void destroy(char **result) {
    // TODO: Implement me!
    if(!result) {
        return;
    }
    char ** temp = result;
    while(*temp) {
        free(*temp);
        *temp = NULL;
        temp++;
    }
    free(result);
    result = NULL;
}



