/**
 * mad_mad_access_patterns
 * CS 241 - Spring 2022
 */
#include "tree.h"
#include "utils.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/
int find_word(int offset, char * addr,  char * word);
int main(int argc, char **argv) {
    if(argc < 3) {
      printArgumentUsage();
      exit(1);
    }

    // FILE * input = fopen(argv[1], "r");
    int input = open(argv[1], O_RDONLY);
    if(!input) {
      openFail(argv[1]);
      exit(1);
    }
    struct stat sb;
    if (fstat(input, &sb) == -1) {
      openFail(argv[1]);
      exit(1);
    }         

    size_t length = sb.st_size;
    char *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, input, 0);
    if((void*)addr == NULL) {
      mmapFail(argv[1]);
      exit(3);
    }
    if(strncmp(addr, BINTREE_HEADER_STRING, BINTREE_ROOT_NODE_OFFSET) != 0) {
      formatFail(argv[1]);
      exit(2);
    }

    for(int i = 2; i < argc; i++) {
      int found = find_word(BINTREE_ROOT_NODE_OFFSET, addr, argv[i]);
      if(!found) {
        printNotFound(argv[i]);
      }
      // puts(argv[i]);
    }
    close(input);
    return 0;
}

int find_word(int offset, char * addr,  char * word) {
  if(!offset) return 0;
  BinaryTreeNode * bstNode = (BinaryTreeNode*)(addr + offset);
  if(strcmp(word, bstNode->word) == 0) {
    printFound(word, bstNode->count, bstNode->price);
    return 1;
  } else if (strcmp(word, bstNode->word) < 0) {
    if(find_word(bstNode->left_child, addr, word)) return 1;
  } else {
    if(find_word(bstNode->right_child, addr, word)) return 1; 
  }
  return 0;
}