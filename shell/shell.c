/**
 * shell
 * CS 241 - Spring 2022
 */
#include "format.h"
#include "shell.h"
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "includes/vector.h"
#include "includes/sstring.h"
#include "signal.h"
#include <getopt.h>
#include <time.h>

typedef struct process {
    char *command;
    pid_t pid;
} process;

static char * currentDirectory;
static char buf[PATH_MAX + 1];
static char * history_file_name = NULL;
static struct vector* commands = NULL;
static struct vector* processes = NULL; 
static int input = 0;
static int output = 0;
static int append = 0;
static char * output_filename = NULL;
static char * input_filename = NULL;
static char * append_filename = NULL;

void skip_lines(FILE *fp, int numlines);
int change_directory(char *);
void history_file(char * filename, int);
void history(char * command);
void nth_command(char * command);
void history_command(char * command);
int external_command(char * command);
void sigint_handler(int sig);
void write_to_file();
void command_file(char * filename, int);
int operator(char * command);
int is_background(char ** command);
void ps();
void add_process_to_vector(char * command, pid_t pid);
char * concat(char ** argv);
int check_input(char * command);
int check_output(char * command);
int check_append(char * command);
void _kill(char * command);
void _stop(char * command);
void _cont(char * command);


int shell(int argc, char *argv[]) {
    commands = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    processes = shallow_vector_create();

    char * shell_process = concat(argv);
    add_process_to_vector(shell_process, getpid());





    int option;
    // if(argc > 3) {
        int x = 0;
        while((option = getopt(argc, argv, "f:h:")) != -1 ) {
            if(option == 'h') {
                history_file(optarg, x);
                if(x == 0) {x = 1;}
            }
            if(option == 'f') {
                command_file(optarg, x);
                size_t i = 0;
                while(i < vector_size(commands)) {
                    char * command = vector_get(commands, i);
                    if(strstr(command, "&&") || strstr(command, "||") || strstr(command, ";")) {
                        operator(command);
                    } else if(strncmp(command, "cd", 2) == 0) {
                        change_directory(command);
                    } else {
                        external_command(command);
                    }
                    i++;
                }
                if(x == 0) {x = 1;}
            }
            if(option == '?') {
                print_usage();
            }
        }
    // }



    char *command;
    int eof;
    size_t len = 0;
    currentDirectory = getcwd(buf, PATH_MAX + 1);

    while(1) {
        signal(SIGINT, sigint_handler);
        signal(SIGCHLD, sigint_handler);
        print_prompt(currentDirectory, getpid());

        //Parse input
        eof = getline(&command, &len, stdin);
        if(eof == -1){break;}
        if(strcmp(command, "\n") == 0) {
            continue;
        }
        char* newline = strchr(command, '\n');
        if(newline) *newline = 0;

        if(strstr(command, "&&") || strstr(command, "||") || strstr(command, ";")) {
            operator(command);
        } else if(strncmp(command, "cd", 2) == 0) {
            change_directory(command);
            vector_push_back(commands, command);
        } else if(strncmp(command, "!history", 8) == 0) {
            history(command);
        } else if (strncmp(command, "!", 1) == 0){
            history_command(command);
        } else if (strncmp(command, "#", 1) == 0){
            nth_command(command);
        } else if (strncmp(command, "ps", 2) == 0) {
            ps();
        } else if (strncmp(command, "kill", 4) == 0) {
            _kill(command);
        } else if (strncmp(command, "cont", 4) == 0) {
            _cont(command);
        } else if (strncmp(command, "stop", 4) == 0) {
            _stop(command);
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            input = check_input(command);
            output = check_output(command);
            append = check_append(command);
            external_command(command);
            vector_push_back(commands, command);
        }
    }
    if(history_file_name) {
        write_to_file();
        free(history_file_name);
    }
    return 0;
}


int change_directory(char * command) {
    command += 3;
    int status = chdir(command);
    if(status==-1) {print_no_directory(command);}
    else{print_command_executed(getpid());}
    currentDirectory = getcwd(buf, PATH_MAX + 1);
    return status;
}


void history(char * command) {
    if(strcmp(command, "!history") != 0) {print_invalid_command(command);}
    else {
        print_command_executed(getpid());
        for (size_t i = 0; i < vector_size(commands); i++)
        {
            print_history_line(i, vector_get(commands, i));
        }
    }
}

void history_file(char * filename, int x) {
    history_file_name = malloc(strlen(filename) + 1);
    history_file_name = strcpy(history_file_name, filename);

    if(access(history_file_name, R_OK | F_OK) != -1) {
        FILE *file = fopen(history_file_name, "r");
        char line[256];

        while(fgets(line, sizeof(line), file)) {
            char* newline = strchr(line, '\n');
            if(newline) *newline = 0;
            if(x == 0) {
                vector_push_back(commands, line);
            }
        }
        fclose(file);
    } else {
        print_history_file_error();
    }
    return;
}

void command_file(char * filename, int x) {
    if(access(filename, R_OK | F_OK) != -1) {
        FILE *file = fopen(filename, "r");
        if(file == NULL) {
            print_script_file_error();
            return;
        }
        char line[256];

        while(fgets(line, sizeof(line), file)) {
            char* newline = strchr(line, '\n');
            if(newline) *newline = 0;
            if(x == 0) {
                vector_push_back(commands, line);
            }
        }
        fclose(file);
    } else {
        print_script_file_error();
    }
    return;
}

void nth_command(char * command) {
    command++;
    size_t number = atoi(command);
    if(number >= 0 && number < vector_size(commands)) {
        print_command(vector_get(commands, number));
        char * found_command = malloc(strlen(vector_get(commands, number)) + 1);
        strcpy(found_command, vector_get(commands, number));
        if(strstr(found_command, "&&") || strstr(found_command, "||") || strstr(found_command, ";")) {
            operator(found_command);
        } else if(strncmp(found_command, "cd", 2) == 0) {
            change_directory(found_command);
            vector_push_back(commands, found_command);
        } else {
           external_command(found_command); 
           vector_push_back(commands, found_command);
        }
        free(found_command);
    } else {
        print_invalid_index();
        return;
    }
}

void history_command(char * command){
    command++;
    // print_command_executed(getpid());
    size_t i = vector_size(commands) - 1;
    for(; i > 0; --i) {

        // strncmp(command, vector_get(commands, i), strlen(command));
        if(strncmp(command, vector_get(commands, i), strlen(command)) == 0) {
            char * found_command = malloc(strlen(vector_get(commands, i)) + 1);
            strcpy(found_command, vector_get(commands, i));
            if(strstr(found_command, "&&") || strstr(found_command, "||") || strstr(found_command, ";")) {
                operator(found_command);
            } else if(strncmp(found_command, "cd", 2) == 0) {
                change_directory(found_command);
                return;
            } else {
                external_command(found_command);
            }
            vector_push_back(commands, found_command);
            // free(found_command);
            return;
        }
    } 
    print_no_history_match();
    return;
}

int external_command(char * command) {
    int background = is_background(&command);
    int worked = 0; 
    sstring* c_to_args = cstr_to_sstring(command);
    vector * v_args = sstring_split(c_to_args, ' ');
    char ** args = (char**)vector_front(v_args);

    fflush(stdout);
    pid_t pid = fork();
    add_process_to_vector(command, pid);
    if(pid == -1) {
        print_fork_failed();
        return -1;
    } 
    else if (pid == 0) {
        if (background) {
            if (setpgid(getpid(), getpid()) == -1) {
                print_setpgid_failed();
                fflush(stdout);
                exit(1);
            }
        }

        worked = 1;

        if(output) {
            freopen(output_filename, "w+", stdout);
        }
        if(input) {
            freopen(input_filename, "r", stdin);
        }
        if(append) {
            freopen(append_filename, "a", stdout);
        }

        execvp(args[0], args);
        vector_destroy(v_args);
        sstring_destroy(c_to_args);
        print_exec_failed(command);
        // exit(1);
        return -1; 
    } else if (pid > 0) {
        print_command_executed(pid);
        int status = 0;
        if(background) {
            waitpid(pid, &status, WNOHANG);
        } else {
            int err = waitpid(pid, &status, 0);
            // if (err > 0)
            //     printf("waitpid reaped child pid %d\n", pid);
            if(err == -1) {
                print_wait_failed();
                return -1;
            } else if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0){
                    // exit(1);
                }
                fflush(stdout);
            }
        }
    }
    return worked;
}

void sigint_handler(int sig) {
    if(sig == SIGINT) {
        signal(sig, sigint_handler);
    } else {
        pid_t p = 0;
        int s = 0;
        while((p = waitpid(-1, &s, WNOHANG)) > 0);
    }
}

void write_to_file() {
    FILE *file = fopen(history_file_name, "w+");
    if(file == NULL) {
        print_history_file_error();
    }

    for(size_t i = 0; i < vector_size(commands); i++) {
        char * com = (char*) vector_get(commands, i);
        strcat(com, "\n");
        fputs(com, file);
    }
    fclose(file); 
}

int operator(char * command) {
    vector_push_back(commands, command);
    char * index = NULL;
    char * index2 = NULL;
    char * operator = NULL;
    if((index = strstr(command, "&&")) != NULL) {
        operator = "&&";
        memcpy(index, "\0\0", strlen(operator));  
    } else if((index = strstr(command, "||")) != NULL) {
        operator = "||";
        memcpy(index, "\0\0", strlen(operator));  
    } else if((index = strstr(command, ";")) != NULL) {
        operator = ";";
        if((index2 = strrchr(command, ';')) != NULL) {
            memcpy(index2, "\0", 1);
        }
        memcpy(index, "\0", 1);
    } else {
        return 0;
    }
      
    char * command_one = command;
    char * command_two = index + strlen(operator);
    char * space = NULL;
    if(strcmp(operator, ";") != 0) {
        space = strrchr(command_one, ' ');
        if(space) *space = '\0';
    }
    space = strrchr(command_two, ' ');
    if(space) command_two++;  

    int worked;
    if(strcmp(operator, "&&") == 0) {
        if(strncmp(command_one, "cd", 2) == 0) {
            worked = change_directory(command_one);
            if(worked != -1) {
                if(strncmp(command_two, "cd", 2) == 0) {
                    change_directory(command_two);
                } else {
                    external_command(command_two);
                }
            }
        } else {
            worked = external_command(command_one);
            if(worked != -1) {
                if(strncmp(command_two, "cd", 2) == 0) {
                    change_directory(command_two);
                } else {
                    external_command(command_two);
                }
            }
        }
    } else if(strcmp(operator, "||") == 0) {
        if(strncmp(command_one, "cd", 2) == 0) {
            worked = change_directory(command_one);
            if(worked == -1) {
                if(strncmp(command_two, "cd", 2) == 0) {
                    change_directory(command_two);
                } else {
                    external_command(command_two);
                }
            }
        } else {
           worked = external_command(command_one);
            if(worked == -1) {
                if(strncmp(command_two, "cd", 2) == 0) {
                    change_directory(command_two);
                } else {
                    external_command(command_two);
                }
            }
        }
    } else {
        if(strncmp(command_one, "cd", 2) == 0) {
            change_directory(command_one);
        } else {
            external_command(command_one);
            // printf("%s\n", command_one);
        }
        if(strncmp(command_two, "cd", 2) == 0) {
            change_directory(command_two);
        } else {
            external_command(command_two);
        }
    }

    return 1;
}

int is_background(char ** command){

    char ** temp = command;
    int index = 0;
    while(*temp != NULL) {
        index ++;
        temp++;
    }

    char * end_char = command[index - 1];
    size_t chars_past_end = strlen(end_char);
    if(end_char[chars_past_end - 1] == '&') {
        if(chars_past_end == 1){
            free(end_char);
            command[index - 1] = NULL;
        } else {
           end_char[chars_past_end - 2] = '\0';
        }
        return 1;
    }
    return 0;
}

void ps() {
    print_process_info_header();
    // int pid;
    // long int NLWP;
    // unsigned long int vsize;
    // char state;
    char start_str[100];
    char time_str[10];
    unsigned long long int btime = 0;
    char filename[1000];    


    sprintf(filename, "/proc/stat");
    FILE *fp = fopen(filename, "r");
    int count = 0;
    if(fp != NULL) {
        char temp[1000];
        while(fgets(temp, sizeof(temp), fp) != NULL) {
            // printf("%s\n", temp);
            if(count == 7) {
                sscanf(temp, "%*s %llu", &btime);
                break;
            }
            else
                count++;
                
        }
    }
    fclose(fp);
    // fscanf(fp, "%s", temp);
    // printf("%llu\n", btime);




    for (size_t i = 0; i < vector_size(processes); i++)
    {
    
        process *p = (process*)(vector_get(processes, i));
        sprintf(filename, "/proc/%d/stat", p->pid);
        FILE *f  = fopen(filename, "r");

        if(f == NULL) {
            vector_erase(processes, i);
        } else {
            process_info* pinfo = malloc(sizeof(process_info));
            size_t utime;
            size_t stime;
            unsigned long long int starttime;
    
            fscanf(f, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu %*ld %*ld %*ld %*ld %ld %*ld %llu %lu", 
          &pinfo->state, &utime, &stime, &pinfo->nthreads, &starttime, &pinfo->vsize);
            pinfo->pid = p->pid;
            pinfo->vsize /= 1024;

            starttime /= sysconf(_SC_CLK_TCK);
            unsigned long long int tt = starttime + btime;
            time_t start_t = (time_t)tt;
            struct tm *st = localtime(&start_t);
            time_struct_to_string(start_str, 100, st);
            pinfo->start_str = start_str;

            utime /= sysconf(_SC_CLK_TCK);
            stime /= sysconf(_SC_CLK_TCK);

            size_t total_time = (utime + stime);
            time_t total_time_t = (time_t)total_time;
            struct tm *t = localtime(&total_time_t);
            size_t minutes = t->tm_min;
            size_t seconds = t->tm_sec;
            execution_time_to_string(time_str, 10, minutes, seconds);
            pinfo->time_str = time_str;
            pinfo->command = p->command;
            print_process_info(pinfo);
            free(pinfo);
        }
    }
    
}

char * concat(char ** argv) {
    char * ret = malloc(1);
    *ret = '\0';
    char ** temp = argv;
    while(*temp != NULL) {
        ret = realloc(ret, sizeof(ret) + sizeof(*temp) + 2);
        strcat(ret, *temp);
        strcat(ret, " ");
        temp++;
    }
    ret[strlen(ret) - 1] = '\0';
    return ret;
}

void add_process_to_vector(char * command, pid_t pid) {
    // for(size_t i = 0; i < vector_size(processes); i++) {
    //     process *p = vector_get(processes, i);
    //     if(p->pid == pid) {
    //         p->command = command;
    //     }
    // }
    process *p = malloc(sizeof(process));
    
    p->command =  strdup(command);
    p->pid = pid;
    vector_push_back(processes, p);
}


void skip_lines(FILE *fp, int numlines)
{
    int cnt = 0;
    char ch;
    while((cnt < numlines) && ((ch = getc(fp)) != EOF))
    {
        if (ch == '\n')
            cnt++;
    }
    return;
}

int check_output(char * command) {
    char * index = NULL;
    if((index = strstr(command, ">")) != NULL) {
        if(strncmp(index, "> ", 2) == 0) {
            output_filename = &index[2];
            memcpy(index - 1, "\0\0", 2);
            return 1;
        }
    }
    return 0;
}

int check_input(char * command) {
    char * index = NULL;
    if((index = strstr(command, "<")) != NULL) {
        if(strncmp(index, "< ", 2) == 0) { 
            input_filename = &index[2];
            memcpy(index - 1, "\0\0", 2);
            return 1;
        }
    }
    return 0;
}

int check_append(char * command) {
    char * index = NULL;
    if((index = strstr(command, ">")) != NULL) {
        if(strncmp(index, ">>", 2) == 0) { 
            append_filename = &index[3];
            memcpy(index - 1, "\0\0\0", 3);
            return 1;
        }
    }
    return 0;

}

void _kill(char * command) {
    vector_push_back(commands, command);
    command += 4;
    int pid = atoi(command);
    for(size_t i = 0; i < vector_size(processes); i++) {
        process * p = vector_get(processes, i);
        if(p->pid == pid){
            kill(pid, SIGKILL);
            vector_erase(processes, i);
            print_killed_process(pid, p->command);
            return;
        }
    }
    print_no_process_found(pid);
}

void _stop(char * command) {
    vector_push_back(commands, command);
    command += 4;
    int pid = atoi(command);
    for(size_t i = 0; i < vector_size(processes); i++) {
        process * p = vector_get(processes, i);
        if(p->pid == pid){
            kill(pid, SIGSTOP);
            // vector_erase(processes, i);
            print_stopped_process(pid, p->command);
            return;
        }
    }
    print_no_process_found(pid);
}
void _cont(char * command) {
    vector_push_back(commands, command);
    command += 4;
    int pid = atoi(command);
    for(size_t i = 0; i < vector_size(processes); i++) {
        process * p = vector_get(processes, i);
        if(p->pid == pid){
            kill(pid, SIGCONT);
            // vector_erase(processes, i);
            print_continued_process(pid, p->command);
            return;
        }
    }
    print_no_process_found(pid);
}