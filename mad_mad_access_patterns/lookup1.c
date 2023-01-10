/**
 * mad_mad_access_patterns
 * CS 241 - Spring 2022
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/
char * read_word(BinaryTreeNode *bstNode, FILE * input);
int find_word(int offset, FILE * file, char * word);

int main(int argc, char **argv) {
    if(argc < 3) {
      printArgumentUsage();
      exit(1);
    }
    // char ** words = argv + 2;
    
    // BinaryTreeNode bstNode;

    FILE * input = fopen(argv[1], "r");
    if(!input) {
      openFail(argv[1]);
      exit(1);
    }

    char bstCheck[4];
    fread(&bstCheck, sizeof(char), 4, input);
    if(strcmp(bstCheck, "BTRE") != 0) {
      formatFail(argv[1]);
      exit(2);
    }
    // fseek(input, BINTREE_ROOT_NODE_OFFSET, SEEK_SET);
    for(int i = 2; i < argc; i++) {
      int found = find_word(BINTREE_ROOT_NODE_OFFSET, input, argv[i]);
      if(!found) {
        printNotFound(argv[i]);
      }
      // puts(argv[i]);
    }
    fclose(input);
    return 0;
}


int find_word(int offset, FILE * file, char * word) {
  if(!offset) return 0;
  fseek(file, offset, SEEK_SET);
  // fseek(file, BINTREE_ROOT_NODE_OFFSET, SEEK_SET);
  BinaryTreeNode bstNode;
  fread(&bstNode, sizeof(BinaryTreeNode), 1, file);
  fseek(file, offset+sizeof(BinaryTreeNode), SEEK_SET);
  char word_node[10];
  fread(word_node, 10, 1, file);
  if(strcmp(word, word_node) == 0) {
    printFound(word, bstNode.count, bstNode.price);
    return 1;
  } else if (strcmp(word, word_node) < 0) {
    if(find_word(bstNode.left_child, file, word)) return 1;
  } else {
    if(find_word(bstNode.right_child, file, word)) return 1; 
  }
  return 0;
}


