/**
 * nonstop_networking
 * CS 241 - Spring 2022
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

/**
 * Read the first four bytes from socket and transform it into ssize_t
 *
 * Returns the size of the incomming message,
 * 0 if socket is disconnected, -1 on failure
 */
ssize_t get_message_size(int socket);

/**
 * Writes the bytes of size to the socket
 *
 * Returns the number of bytes successfully written,
 * 0 if socket is disconnected, or -1 on failure
 */
ssize_t write_message_size(size_t size, int socket);

/**
 * Attempts to read all count bytes from socket into buffer.
 * Assumes buffer is large enough.
 *
 * Returns the number of bytes read, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t read_all_from_socket(int socket, char *buffer, size_t count);

/**
 * Attempts to write all count bytes from buffer to socket.
 * Assumes buffer contains at least count bytes.
 *
 * Returns the number of bytes written, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count);

int print_err(size_t bytes_rd, size_t size);

char *size_to_string(size_t size);

size_t string_to_size(char *size_str);