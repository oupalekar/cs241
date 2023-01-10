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
#include "includes/vector.h"
#include "includes/dictionary.h"
#include <sys/epoll.h>
#include "common.h"
#include <fcntl.h>

typedef struct client_t {
    int header;
    char *buffer;
    size_t buffer_len;
    int sock_fd;
    verb method;
    char *file_name;
    int file_fd;
    size_t file_size;
    size_t byte_read;
    int finished;
} client_info;

int setup_server(char * port);
void handle_signal(int signal);
size_t get(client_info * ci);
int put_(client_info * ci);
int list(client_info * ci);
int read_from_client(client_info * ci);
void shift_forward(char *buffer, size_t content_size, size_t offset);
int read_header(client_info * ci);
size_t get_index(char * filename);
int delete_file(char * file_name);
int header_output(int fd, const char* mesg); 
size_t write_to_socket(int sock_fd, char *buffer, size_t size, int strict);
size_t handle_list(client_info *ci);
int str_to_verb(char * method);
void end_program();


#define MAX_CLIENTS 100
#define BUFFER 2048
#define SIZE_STR 8

static char* directory = NULL;
static vector *files = NULL;
static dictionary *clients = NULL;

int main(int argc, char **argv) {
    // good luck!
    if(argc != 2) {
       printf("\nUsage: ./server <port>\n\n");
       exit(1); 
    }

    char template[] = "XXXXXX";

    directory = mkdtemp(template);
    if(directory == NULL) {
        perror("mkdtemp");
        exit(1);
    }

    print_temp_directory(directory);

    if(chdir(directory) == -1) {
        perror("chdir");
        rmdir(directory);
        exit(1);
    }

    files = string_vector_create();
    clients = int_to_shallow_dictionary_create();


    signal(SIGINT, handle_signal);
    signal(SIGPIPE, handle_signal);

    int serverfd = setup_server(argv[1]);
    if(serverfd == -1) {
        perror("serverfd");
        end_program();
        exit(1);
    }

    if(listen(serverfd, MAX_CLIENTS) == -1) {
        perror("listen");
        end_program();
        exit(1);
    }

    int epoll = epoll_create(1);
    if(epoll == -1) {
        perror("epoll");
        end_program();
        exit(1);
    }

    struct epoll_event ev, events[MAX_CLIENTS];
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = serverfd;

    if(epoll_ctl(epoll, EPOLL_CTL_ADD, serverfd, &ev) == -1) {
        perror("epoll_ctl");
        end_program();
        exit(1);
    }
    while(1) {
        int nfds = epoll_wait(epoll, events, MAX_CLIENTS, -1);
        if(nfds == -1) {
            perror("nfds");
            end_program();
            exit(1);
        }

        for(int n = 0; n < nfds; ++n) {
            if(events[n].data.fd == serverfd) {
                while(1) {
                    int connected_socket = accept(serverfd, NULL, NULL);
                    if(connected_socket < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            perror("accept");
                            end_program();
                            exit(1);
                        }
                    }
                    int flags = fcntl(connected_socket, F_GETFL);
                    if(flags == -1 || fcntl(serverfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                        perror("fcntl");
                        end_program();
                        exit(1);
                    }
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connected_socket;
                    if(epoll_ctl(epoll, EPOLL_CTL_ADD, connected_socket, &ev) == -1) {
                        perror("epoll_ctl");
                        end_program();
                        exit(1);
                    }

                    client_info * ci = malloc(sizeof(client_info));
                    ci->header = 0;
                    ci->buffer = malloc(BUFFER);
                    ci->buffer_len = 0;
                    ci->sock_fd = connected_socket;
                    ci->method = V_UNKNOWN;
                    ci->file_name = NULL;
                    ci->file_fd = -1;
                    ci->file_size = -1;
                    ci->byte_read = 0;
                    ci->finished = 0;
                    dictionary_set(clients, &connected_socket, ci);
                }
            } else {
                int client_fd = events[n].data.fd;
                client_info *ci = dictionary_get(clients, &client_fd);
                if(read_from_client(ci) == -1) {
                    perror("read error");
                    end_program();
                    exit(1);
                }

                if(epoll_ctl(epoll, EPOLL_CTL_DEL, client_fd, events) == -1) {
                    end_program();
                    exit(1);
                } else {
                    shutdown(client_fd, SHUT_RDWR);
                    close(client_fd);
                    free(ci->buffer);
                    free(ci->file_name);
                    if(ci->file_fd != -1) {
                        close(ci->file_fd);
                    }
                    free(ci);
                    dictionary_remove(clients, (void*)&client_fd);
                }
            }
        }
    }
    close(serverfd);
    vector_destroy(files);
    return 0;
}


int setup_server(char * port) {
    int socketfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(socketfd == -1) {
        perror("Socket");
        return -1;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    // memset(&res, 0, sizeof(res));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int retval = getaddrinfo(NULL, port, &hints, &res);
    if(retval) {
        const char *mesg = gai_strerror(retval);
        fprintf(stderr, "%s\n", mesg);
        close(socketfd);
        freeaddrinfo(res);
        return -1;
    }

    int optval = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(res);
        close(socketfd);
        return -1;
    }
    freeaddrinfo(res);
    return socketfd;
}


void handle_signal(int signal) {
    if(signal == SIGINT) {
        end_program();
        exit(1);
    }
}

int read_from_client(client_info * ci) {
    size_t header_len = 0;
    while(1) {
        size_t byte_read = read(ci->sock_fd, ci->buffer + ci->buffer_len, BUFFER - ci->buffer_len - 1);
        if(byte_read == (size_t)-1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            } else {
                perror("read");
                end_program();
                exit(1);
            }
        } else if(byte_read > 0) {
            ci->byte_read += byte_read;
            ci->buffer_len += byte_read;
            ci->buffer[ci->buffer_len] = '\0';
            if(!ci->header) {
                header_len = read_header(ci);
                if(header_len == (size_t)-1) {
                    header_output(ci->sock_fd, err_bad_request);
                    return 0;
                } else if (header_len > 0) {
                    ci->header = 1;
                    shift_forward(ci->buffer, ci->buffer_len, header_len);
                    ci->buffer_len -= header_len;
                } else {
                    continue;
                }
            }

            size_t bytes_consumed = 0;
            if(ci->method == PUT) {
                bytes_consumed = put_(ci);
                if (bytes_consumed == (size_t)-1) {
                    return -1;
                } else {
                    shift_forward(ci->buffer, ci->buffer_len, bytes_consumed);
                    ci->buffer_len -= bytes_consumed;
                }
            } else if (ci->method == V_UNKNOWN) {
                header_output(ci->sock_fd, err_bad_request);
                return -1;
            } else {
                if(ci->buffer_len > 0) {
                    header_output(ci->sock_fd, err_bad_request);
                    return -1;
                }
                if(ci->method == GET) {
                    return (get(ci) == (size_t)-1) ? -1 : 0;
                } else if(ci->method == LIST) {
                    return (list(ci) == -1) ? -1 : 0;
                } else {
                    if (delete_file(ci->file_name) == -1) {
                        header_output(ci->sock_fd, err_no_such_file);
                    } else {
                        header_output(ci->sock_fd, NULL);
                    }
                    return 0;
                }
            }
        } else { 
            ci->finished = 1;
            if(ci->method == PUT) {
                if(ci->byte_read != ci->file_size + header_len+ SIZE_STR) {
                    if(ci->byte_read > ci->byte_read + SIZE_STR) {
                        print_received_too_much_data();
                    } else {
                        print_too_little_data();
                    }
                    close(ci->file_fd);
                    unlink(ci->file_name);
                    size_t file_index = get_index(ci->file_name);
                    if(file_index != (size_t)-1) {
                        vector_erase(files, file_index);
                    }
                    header_output(ci->sock_fd, err_bad_file_size);
                } else {
                    header_output(ci->sock_fd, NULL);
                }
            } else {
                header_output(ci->sock_fd, NULL);
            }
            return 0;
        }
    }
}


void shift_forward(char *buffer, size_t buffer_size, size_t offset) {
    if (offset == buffer_size) {
        *buffer = '\0';
    } else if (offset == 0) {
        return;
    } else {
        char temp[buffer_size - offset];
        memcpy(temp, buffer + offset, buffer_size - offset);
        memcpy(buffer, temp, buffer_size - offset);
    }
}

int read_header(client_info * ci) {
    char clone[1024];
    memcpy(clone, ci->buffer, 1024);
    char * end = strstr(clone, "\n");

    if(end == NULL) {
        if(ci->buffer_len >= 1024) {
            return -1;
        } else {
            return 0;
        }
    } else if (end - clone + 1 > 1024) {
        return -1;
    }

    *end = '\0';

    char * method = strtok(clone, " ");
    char * file_name = strtok(NULL, " ");
    char * excess = strtok(NULL, " ");
    if(method == NULL || excess != NULL) {
        // LOG("in here");
        return -1;
    } else {
        ci->method = str_to_verb(method);
        if(ci->method == V_UNKNOWN) {
            return -1;
        } else if(ci->method == LIST) {
            if(file_name != NULL) {
                return -1;
            }
        } else {
            if(file_name == NULL) {
                return -1;
            } else {
                ci->file_name = malloc(strlen(file_name) + 1);
                strcpy(ci->file_name, file_name);
            }
        }
    }
    return end - clone + 1;
}

size_t get(client_info * ci) {
    if(get_index(ci->file_name) == (size_t)-1) {
        header_output(ci->sock_fd, err_no_such_file);
        return 0;
    } else {
        ci->file_fd = open(ci->file_name, O_RDONLY);
        if(ci->file_fd < 0) {
            perror("open");
            return -1;
        }
        header_output(ci->sock_fd, NULL);

        struct stat st;
        stat(ci->file_name, &st);
        char *size_str = size_to_string(st.st_size);
        memcpy(ci->buffer, size_str, SIZE_STR);
        ci->buffer_len = SIZE_STR;
        free(size_str);

        while (1) {
            size_t byte_read = read(ci->file_fd, ci->buffer + ci->buffer_len,
                                    BUFFER- ci->buffer_len - 1);
            if (byte_read == (size_t)-1) {
                if (errno != EINTR) {
                    close(ci->file_fd);
                    return -1;
                }
            } else if (byte_read > 0) {
                ci->buffer_len += byte_read;
                ci->buffer[ci->buffer_len] = '\0';
                size_t byte_write = write_to_socket(ci->sock_fd, ci->buffer, ci->buffer_len, 0);
                if (byte_write == (size_t)-1) {
                    close(ci->file_fd);
                    return -1;
                } else if (byte_write > 0) {
                    shift_forward(ci->buffer, ci->buffer_len, byte_write);
                    ci->buffer_len -= byte_write;
                }
            } else {
                return (write_to_socket(ci->sock_fd, ci->buffer, ci->buffer_len, 1) == (size_t)-1) ? -1 : 0;
            }
        }
    }
}

size_t get_index(char * filename) {
    for(size_t i = 0; i < vector_size(files); i++) {
        if(strcmp(filename, (char*)vector_get(files, i)) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

int delete_file(char * file_name) {
    size_t index = get_index(file_name);
    if(index != (size_t)-1) {
        vector_erase(files, index);
        unlink(file_name);
        return 0;
    }
    return -1;
}

int header_output(int fd, const char* mesg) {
    char buffer[1024 + 1];
    if(!mesg) {
        sprintf(buffer, "OK\n");
    } else {
        sprintf(buffer, "ERROR\n%s\n", mesg);
    }
    return write_to_socket(fd, buffer, strlen(buffer), 1);
}


size_t write_to_socket(int sock_fd, char *buffer, size_t size, int strict) {
    size_t byte_write_total = 0;
    while (byte_write_total < size) {
        size_t byte_write = write(sock_fd, buffer + byte_write_total, size - byte_write_total);
        if (byte_write == (size_t)-1) {
            if ((errno == EAGAIN || errno == EWOULDBLOCK) && strict == 0) {
                return byte_write_total;
            } else if (errno != EINTR) {
                perror("write");
                return -1;
            }
        } else if (byte_write > 0) {
            byte_write_total += byte_write;
        }
    }
    return byte_write_total;
}

int list(client_info *ci) {
    header_output(ci->sock_fd, NULL);
    size_t num_files = vector_size(files);
    size_t total_bytes = 0;
    for (size_t i = 0; i < num_files; ++i) {
        total_bytes += strlen(vector_get(files, i));
        if (i + 1 != num_files) {
            total_bytes += 1;
        }
    }
    size_t i = 0;
    char *size_str = size_to_string(total_bytes);
    memcpy(ci->buffer, size_str, 9);
    ci->buffer_len = SIZE_STR;
    free(size_str);
    while (i < num_files) {
        char *file_name = vector_get(files, i);
        if ((BUFFER - ci->buffer_len) > (strlen(file_name) + 1)) {
            strcpy(ci->buffer + ci->buffer_len, file_name);
            ci->buffer_len += strlen(file_name);
            if (i + 1 != num_files) {
                ci->buffer[ci->buffer_len++] = '\n';
            }
            ci->buffer[ci->buffer_len] = '\0';
            ++i;
        } else {
            size_t byte_write = write_to_socket(ci->sock_fd, ci->buffer, ci->buffer_len, 0);
            if (byte_write == (size_t)-1) {
                return -1;
            }
            shift_forward(ci->buffer, ci->buffer_len, byte_write);
            ci->buffer_len -= byte_write;
        }
    }
    size_t byte_write = write_to_socket(ci->sock_fd, ci->buffer, ci->buffer_len, 1);
    if (byte_write == (size_t)-1) {
        return -1;
    }
    shift_forward(ci->buffer, ci->buffer_len, byte_write);
    ci->buffer_len -= byte_write;    
    return 0;
}

int str_to_verb(char * method) {
    if(strcmp(method, "GET") == 0) {
        return GET;
    } else if(strcmp(method, "LIST") == 0) {
        return LIST;
    } else if(strcmp(method, "PUT") == 0) {
        return PUT;
    } else if(strcmp(method, "DELETE") == 0) {
        return DELETE;
    } else {
        return V_UNKNOWN;
    }
}


int put_(client_info * ci) {
    if (ci->file_fd == -1) {
        ci->file_fd = open(ci->file_name, O_WRONLY | O_CREAT | O_TRUNC, 0755);
        if (ci->file_fd < 0) {
            perror("open");
            return -1;
        }
        if (get_index(ci->file_name) == (size_t)-1) {
            vector_push_back(files, ci->file_name);
        }
    }
    size_t byte_consumed = 0;
    if (ci->file_size == (size_t)-1) {
        if (ci->buffer_len < SIZE_STR) {
            return 0;
        } else {
            ci->file_size = string_to_size(ci->buffer);
            byte_consumed += SIZE_STR;
        }
    }
    // write to file
    size_t byte_write = write_all_to_socket(ci->file_fd, ci->buffer + byte_consumed, ci->buffer_len - byte_consumed);
    if (byte_write == (size_t)-1 && errno != EINVAL) {
        perror("write");
        return -1;
    }
    return byte_consumed + byte_write;
}

void end_program() {
    if(directory != NULL) {
        size_t num_files = vector_size(files);
        for(size_t i = 0; i < num_files; i++) {
            unlink((char*)vector_get(files, i));
        }
        chdir("..");
        rmdir(directory);
    }
    if(!files) {
        vector_destroy(files);
    }
    if(!clients) {
        vector *keys = dictionary_keys(clients);
        size_t num_keys = vector_size(keys);
        for(size_t i = 0; i < num_keys; i++) {
            client_info * ci = dictionary_get(clients, vector_get(keys, i));
            free(ci->buffer);
            free(ci->file_name);
            free(ci);
        }
        vector_destroy(keys);
        dictionary_destroy(clients);
    }
}