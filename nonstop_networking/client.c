/**
 * nonstop_networking
 * CS 241 - Spring 2022
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);
void connect_to_server(char *, char *);
void read_from_server();
void write_to_server();


static verb command = V_UNKNOWN;
static char ** args = NULL;
// static int local_fd = -1;
static int server_fd = -1;



int main(int argc, char **argv) {
    // Good luck!
    args = parse_args(argc, argv);
    command = check_args(args);
    // printf("%u\n", command);
    connect_to_server(args[0], args[1]);
    // puts("PUT");
    write_to_server();
    if(shutdown(server_fd, SHUT_WR) == -1) {
        perror(NULL);
    }
    read_from_server();
    shutdown(server_fd, SHUT_RD);
    
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}

void connect_to_server(char * host, char * port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {perror(NULL); exit(1);}
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int retval = getaddrinfo(host, port, &hints, &res);
    if(retval) {
        const char *mesg = gai_strerror(retval);
        fprintf(stderr, "%s\n", mesg);
        freeaddrinfo(res);
        exit(1);
    }

    if(connect(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror(NULL);
        freeaddrinfo(res);
        exit(1);
    }
    freeaddrinfo(res);
}


void read_from_server() {
    char * buf = calloc(1, strlen("OK\n") + 1);
    size_t ok_check_bytes = read_all_from_socket(server_fd, buf, strlen("OK\n"));
    if(strcmp(buf, "OK\n") == 0) {
        fprintf(stdout, "connected w/ server\n");
        if(command == PUT || command == DELETE) {
            print_success();
        } else if(command == GET) {
            // fprintf(stdout, "here\n");
            FILE * local = fopen(args[4], "a+");
            if(!local){perror(NULL); exit(1);}
            // fprintf(stdout, "here\n");
            size_t size;
            read_all_from_socket(server_fd, (char*)&size, sizeof(size_t));
            // fprintf(stdout, "%zu\n", size);
            size_t bytes_to_read = 0;
            while (bytes_to_read < size+5) {
                size_t size_hd = (size+5-bytes_to_read) > 1024 ? 1024 : (size+5-bytes_to_read);
                char buffer_f[1024+1] = {0};
                size_t rc = read_all_from_socket(server_fd, buffer_f, size_hd);
                fwrite(buffer_f, 1, rc, local);
                // fprintf(stdout, "%s", buffer_f);
                bytes_to_read += rc;
                if (rc == 0)
                    break;
            }
            if (print_err(bytes_to_read, size)) exit(1);
            fclose(local);
        } else if (command == LIST) {
            // fprintf(stderr, "here");
            size_t size;
            read_all_from_socket(server_fd, (char*)&size, sizeof(size_t));
            char buffer[size + 5 + 1];
            memset(buffer, 0, size + 5 + 1);
            size_t bytes_read = read_all_from_socket(server_fd, buffer, size + 5);
            if(print_err(bytes_read, size) == 1) {
                exit(1);
            }
            fprintf(stdout, "%s\n", buffer);
        }
    } else {
        buf = realloc(buf, strlen("ERROR\n") + 1);
        read_all_from_socket(server_fd, buf + ok_check_bytes, strlen("ERROR\n") - ok_check_bytes);
        if(strcmp(buf, "ERROR\n") == 0) {
            // fprintf(stderr, "%s\n", buf);
            char err[20] = {0};
            if (!read_all_from_socket(server_fd, err, 20)) {
                print_connection_closed();
            }
            print_error_message(err);
        } else {
            print_invalid_response();
        }
    }
    free(buf);
}

void write_to_server() {
    char * message = NULL;
    if(command == LIST) {
        message = calloc(1, strlen(args[2]) + 2);
        sprintf(message, "%s\n", args[2]);
    } else {
        message = calloc(1, strlen(args[2])+strlen(args[3])+3);
        sprintf(message, "%s %s\n", args[2], args[3]);
    }
    ssize_t len = strlen(message);
    if(write_all_to_socket(server_fd, message, len) < len) {
        // printf("here");
        print_connection_closed();
        exit(1);
    }
    free(message);

    if(command == PUT) {
        // fprintf(stderr,"here");
        struct stat stats;
        if(stat(args[4], &stats) == -1) {
            exit(1);
        }
        // fprintf(stderr, "here\n");
        ssize_t size = stats.st_size;
        write_all_to_socket(server_fd, (char*)&size, sizeof(size_t));
        FILE * local = fopen(args[4], "r");
        ssize_t bytes_to_write = 0;

        while(bytes_to_write < size) {
            ssize_t actual_size = size - bytes_to_write;
            if(size - bytes_to_write > 1024) {
                actual_size = 1024;
            }
            char buffer[actual_size + 1];
            fread(buffer, 1, actual_size, local);
            if (write_all_to_socket(server_fd, buffer, actual_size) < actual_size) {
                print_connection_closed();
                exit(1);
            }
            bytes_to_write += actual_size;
        }
        // printf("here");
        LOG("client write_cmd bytes_write:%zu", bytes_to_write);
        fclose(local);
    }
}
