#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_THREADS 16
#define BASE_SIZE (10 * 1024 * 1024)

void run_program(const char *program, int size, int threads) {
    char command[100];
    sprintf(command, "%s -n %d -t %d >> data.txt", program, size, threads);
    system(command);
}

double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

int main(int argc, char **argv) {
    const char *programs[] = {"./sharing", "./stealing"};
    const int num_programs = sizeof(programs) / sizeof(programs[0]);
    const int num_sizes = 8;
    int threads = 8;
    if (argc > 1){
        threads = atoi(argv[1]);
    }
    int sizes[num_sizes];
    int i;
    for (i = 0; i < num_sizes; i++) {
        sizes[i] = 1 << i;
    }

    struct timeval start_time, end_time;
    double elapsed_time;

    FILE *fp;
    fp = fopen("data.txt", "w");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Header
    fprintf(fp, "Size\tThreads\tProgram\tTime (s)\n");

    for (int size_index = 0; size_index < num_sizes; size_index++) {
        int size = BASE_SIZE * sizes[size_index];
        for (int program_index = 0; program_index < num_programs; program_index++) {
            gettimeofday(&start_time, NULL);
            run_program(programs[program_index], size, threads);
            gettimeofday(&end_time, NULL);
            elapsed_time = get_elapsed_time(start_time, end_time);
            fprintf(fp, "%d\t%d\t%s\t%.6f\n", size, threads, programs[program_index], elapsed_time);
        }
    }

    fclose(fp);

    return 0;
}
