/**
 * finding_filesystems
 * CS 241 - Spring 2022
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
    // Write tests here!    
    file_system * fs = open_fs(argv[1]);
    // // fprintf(stderr, "Got here");
    // // fprintf(stderr, "%s", argv[1]);
    // char * realpath = realpath(argv[2], NULL);
    assert(minixfs_chmod(fs, argv[2], atoi(argv[3])) == -1);
    // execvp(argv[2], &argv[3]);
}
