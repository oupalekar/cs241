/**
 * deepfried_dd
 * CS 241 - Spring 2022
 */
#include "format.h"
#include "stdio.h"
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "stdlib.h"
#include <signal.h>

#define BILLION  1000000000L;
volatile sig_atomic_t static loop = 1;
void handle_signal(int signal);
int main(int argc, char **argv) {
    signal(SIGUSR1, handle_signal);
    if(argc <= 1) {
        fprintf(stderr, "No args\n");
        exit(1);
    }
    char * input_file = NULL;
    char * output_file = NULL;
    int block_size = 512;
    int count = -1;
    int skip_input = 0;
    int skip_output = 0;
    int opt;
    // char * optarg;
    while((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch (opt) {
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'b':
            block_size = atoi(optarg);
            break;
        case 'c':
            count = atoi(optarg);
            break;
        case 'p':
            skip_input = atoi(optarg);
            break;
        case 'k':
            skip_output = atoi(optarg);
            break;
        case '?':
            // fprintf(stderr, "Usage: %s [-i input_file] [-o output_file] [-b block_size] [-c count] [-p skip_input] [-k skip_output]\n",
            //                argv[0]);
            exit(EXIT_FAILURE);
        default:
            exit(1);
        }
    }
    
    char buffer[block_size];
// 
    FILE * input;
    input = fopen(input_file, "r");


    if(!input_file) {
        input = stdin;
    } else if(!input && input_file) {
        print_invalid_input(input_file);
        // fclose(input);
        exit(1);
    }

    FILE * output;
    output = fopen(output_file, "w+");
    if(!output_file) {
        output = stdout;
    }
    if(!output && output_file) {
        print_invalid_output(output_file);
        // fclose(output);
        exit(1);
    }

    fseek(input, block_size * skip_input, SEEK_SET);
    fseek(output, block_size * skip_output, SEEK_SET);

    int count_reader = 0;
    int full_blocks_read = 0;
    int partial_block_read = 0;
    int full_blocks_write = 0;
    int partial_block_write = 0;
    size_t total_bytes_copied = 0;
    struct timespec start, stop;
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }
    while(loop) {
        if(count_reader == count) {
            break;
        }
        if(feof(input)) {
            break;
        }
        int num_bytes_read = fread(&buffer, 1, block_size, input);
        if(num_bytes_read == block_size) {full_blocks_read++;} else if(num_bytes_read == 0){}else{partial_block_read++;}
        // fprintf(stderr, "%zu\n", num_bytes_read); 
        int num_bytes_write = fwrite(&buffer,  sizeof(char), num_bytes_read, output);
        total_bytes_copied+=num_bytes_write;
        if(num_bytes_write == block_size) {full_blocks_write++;}else if(num_bytes_write == 0){} else{partial_block_write++;}
        count_reader++;
    }
    clock_gettime(CLOCK_REALTIME, &stop);

    double time_elapsed = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec)/(double)1000000000;
    print_status_report(full_blocks_read, partial_block_read, full_blocks_write, partial_block_write, total_bytes_copied, time_elapsed);

    return 0;
}

void handle_signal(int signal) {
  loop = 0;
}