/**
 * vector
 * CS 241 - Spring 2022
 */
#include "sstring.h"
#include "vector.h"

int main(int argc, char *argv[]) {
    // TODO create some tests
    vector *v;
    sstring * str = cstr_to_sstring("abcdeffg");
    v = sstring_split(str, 'f');
    for(size_t i = 0; i < vector_size(v); i++) {
        printf("%s\n", (char*)vector_at(v,i));
    }
    printf("%zu\n", vector_size(v));
    // sstring *bang = cstr_to_sstring("!");
    // printf("%d\n", sstring_append(str, bang));
    // printf("%s\n", sstring_to_cstr(str));
    // sstring_substitute(str, 0, "{}", "friend");
    // printf("%s\n", sstring_to_cstr(str));
    // char * x =sstring_slice(str, 2, 5);
    // printf("%s\n", sstring_to_cstr(str));
    // free(x);
    sstring_destroy(str);
    vector_destroy(v);
    // sstring_destroy(bang);
    return 0;
}
