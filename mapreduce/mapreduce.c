/**
 * mapreduce
 * CS 241 - Spring 2022
 */
#include "utils.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/wait.h"
#include "sys/stat.h"

void close_pipes(int *pipes, int size);

int main(int argc, char **argv) {
    if(argc != 6) {
        // fprintf(stderr, "Incorrect amount of arguments\n");
        print_usage();
        exit(1);
    }
    char * input_file = argv[1];
    char * output_file = argv[2];
    char * mapper = argv[3];
    char * reducer = argv[4];
    int mapper_count = atoi(argv[5]);

    pid_t mapper_pids[mapper_count];
    pid_t splitter_pids[mapper_count];

    // Create an input pipe for each mapper.
    int mapper_pipefds[2*mapper_count];
    for(int i = 0; i < (mapper_count); i++){
        pipe(mapper_pipefds + (i*2));
    }
    // Create one input pipe for the reducer.
    int reducer_pipefds[2];
    pipe(reducer_pipefds);
    // Open the output file.
    // int fd_o = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    // Start a splitter process for each mapper.
    for(int i = 0; i < mapper_count; i ++) {
        splitter_pids[i] = fork();
        if(splitter_pids[i] < 0) {
            exit(1);
        } else if (splitter_pids[i] == 0) {
            close(mapper_pipefds[i * 2]);
            int retval = dup2(mapper_pipefds[i* 2 + 1], 1);
            // int retval = dup2(mapper_pipefds[(i * 2) + 1], 1);
            if(retval == -1) {
                exit(2);
            }
            
            // close_pipes(mapper_pipefds, mapper_count * 2);
            // close_pipes(reducer_pipefds, 2 );
            char index[20];
            sprintf(index, "%d", i);
            if (execl("splitter", "splitter", input_file, argv[5], index, NULL) == -1) {
                exit(1);
            }
        }
    }
    // Start all the mapper processes.
    for(int i = 0; i < mapper_count; i++) {
        pid_t pid = mapper_pids[i] = fork();
        if(pid < 0) {
            exit(1);
        } else if(pid == 0) {
            int retval_1 = dup2(mapper_pipefds[(i * 2)], 0); 
            int retval_2 = dup2(reducer_pipefds[1], 1);
            if(retval_1 == -1 || retval_2 == -1) {
                exit(2);
            }
            for(int i = 0; i < (mapper_count * 2); i++){
                close(mapper_pipefds[i]);
            }
            for(int i = 0; i < 2; i++) {
                close(reducer_pipefds[i]);
            }
            int retval = execl(mapper, mapper, NULL);
            if(retval == -1) {
                exit(3);
            }

        }
    }
    // Start the reducer process.
    pid_t pid = fork();
    if(pid < 0) {
        exit(1);
    } else if(pid == 0) {
        FILE * output = freopen(output_file, "w+", stdout);
        if(!output) {
            exit(2);
        }

        int retval = dup2(reducer_pipefds[0], 0);
        if(retval == -1) {
            exit(3);
        }

        for(int i = 0; i < (mapper_count * 2); i++){
            close(mapper_pipefds[i]);
        }
        for(int i = 0; i < 2; i++) {
            close(reducer_pipefds[i]);
        }
        retval = execl(reducer, reducer, NULL);
        if(retval == -1) {
            exit(4);
        }

    } else {
        for(int i = 0; i < (mapper_count * 2); i++){
            close(mapper_pipefds[i]);
        }
        for(int i = 0; i < 2; i++) {
            close(reducer_pipefds[i]);
        } 
    }
    // Wait for the reducer to finish.
    int status;
    for(int i = 0; i < mapper_count; i++) {
        waitpid(splitter_pids[i], &status, 0);
        if(WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if(status != 0) print_nonzero_exit_status("splitter", exit_status);
        }
    }

    for(int i = 0; i < mapper_count; i++) {
        waitpid(mapper_pids[i], &status, 0);
        if(WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if(status != 0) print_nonzero_exit_status(mapper, exit_status);
        }
    }

    waitpid(pid, &status, 0);
    if(WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if(status != 0) print_nonzero_exit_status(reducer, exit_status);
    }
    print_num_lines(output_file);
    return 0;
}