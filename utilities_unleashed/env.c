/**
 * utilities_unleashed
 * CS 241 - Spring 2022
 */
#include "format.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>

char** parse(char **argv, size_t** num, char *** cmd);
void destroy(char **, char **, char **);
int validify_args(char**, char**);

int main(int argc, char *argv[]) {

    if(argc < 4) {
        print_env_usage();
    }
    char ** cmd = NULL;
    size_t * num;
    char **vars = parse(argv, &num, &cmd);

    if(!validify_args(vars, cmd)) {
        destroy(vars, NULL, NULL);
        print_env_usage();
    }

    char *temp = NULL;
    char **key = malloc((*num + 1) * sizeof(char*));
    char **val = malloc((*num + 1) * sizeof(char*));

    pid_t pid = fork();

    if(pid == -1) {
        print_fork_failed();
    } else if (pid == 0) {
        for(size_t i = 0; i < *num; i++) {

            key[i] = vars[i];
            val[i] = strchr(vars[i], '=');
            if(!temp) {
                temp= malloc(val[i] - key[i] + 1);
            } else {
                free(temp);
                temp = malloc(val[i] - key[i] + 1);
            }
            strncpy(temp, key[i], val[i] - key[i]);

            // printf("%c\n", val[0]);
            if(val[i][1] == '%') {
                
                if(!(val[i] = getenv(val[i] + 2))) {
                    val[i] = "";
                }
            }
            if(setenv(temp, val[i] + 1, 1) == -1) {
                print_environment_change_failed();
            }

        }
        execvp(cmd[0], cmd);
        // free(temp);
        destroy(vars, key, val);
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if(!WIFEXITED(status) || WEXITSTATUS(status)) {
            destroy(vars, key, val);
            print_exec_failed();
        }
    }
    destroy(vars, key, val);
    return 0;
}


char** parse(char **argv, size_t** num, char *** cmd) {
    size_t num_vars = 0;
    char ** temp = argv + 1;
    char ** vars = malloc((num_vars + 1) * sizeof(char*));

    while(*(temp)) {
        if(!strcmp(*temp, "--")) {
            *cmd = temp + 1;
            break;
        }
        vars[num_vars] = *temp;
        vars = realloc(vars, (++num_vars + 1) * sizeof(char*));
        temp++;
    }
    vars[num_vars] = NULL;
    *num = &num_vars;
    return vars;
}


void destroy(char ** vars, char ** keys, char ** vals) {
    if(vals){
        free(vars);
        vars = NULL;
    }
    if(keys){
        free(keys);
        keys = NULL;
    }
    if(vals){
        free(vals);
        vals = NULL;
    }
}

int validify_args(char ** vars, char ** cmd) {
    if(*cmd == NULL || cmd == NULL) {
        return 0;
    }
    char ** temp = vars;
    int count = 0;
    while(*temp) {
        int equals_found = 0;
        for(size_t i = 0; i < strlen(*temp); i++) { 
            char c = (*temp)[i];
            if(isalpha(c) || isdigit(c) || c == '_' || c =='=') {
                if(c == '=') {
                    if(equals_found) {
                        return 0;
                    }
                    equals_found = 1;
                }
                continue;
            } else if (count > 1 && c == '%') {
                continue;
            } else {
                return 0;
            }
        }
        temp++;
        count++;
    }
    return 1;
}