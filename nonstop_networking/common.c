/**
 * nonstop_networking
 * CS 241 - Spring 2022
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "format.h"

#include "common.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, sizeof(size_t));
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    ssize_t mesg_size = htonl(size);
    return write_all_to_socket(socket, (char*)&mesg_size, MESSAGE_SIZE_DIGITS);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    ssize_t tot = 0;
    // printf("Socket %d\n", socket);
    while(tot < (ssize_t)count) {
        ssize_t retval = read(socket, buffer + tot, count - tot);
        // printf("%zd\n", retval);
        if (retval > 0) {
            tot += retval;
        } else if (retval == 0) {
            break;
        } else if (retval == -1 && errno != EINTR) {
            return -1;
        }
    }
    return tot;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t tot = 0;
    // printf("Socket %d\n", socket);
    while(tot < (ssize_t)count) {
        ssize_t retval = write(socket, buffer + tot, count - tot);
        if (retval > 0) {
            tot += retval;
        } else if (retval == 0) {
            break;
        } else if (retval == -1 && errno != EINTR) {
            return -1;
        } 
    }
    return tot;
}


int print_err(size_t bytes_rd, size_t size) {
  if (bytes_rd == 0 && bytes_rd != size) {
    print_connection_closed();
    return 1;
  } else if (bytes_rd < size) {
    print_too_little_data();
    return 1;
  } else if (bytes_rd > size) {
    print_received_too_much_data();
    return 1;
  }
  return 0;
}

char *size_to_string(size_t size) {
    char *size_str = malloc(9);
    for (size_t i = 0; i < 8; ++i) {
        size_str[i] = size & 0xFF;
        size = size >> 8;
    }
    size_str[8] = '\0';
    return size_str;
}

size_t string_to_size(char *size_str) {
    size_t size = 0;
    for (size_t i = 0; i < 8; ++i) {
        size += ((unsigned char)size_str[i]) << (i * 8);
    }
    return size;
}