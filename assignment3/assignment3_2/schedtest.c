#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_PROCESSES 10000

void set_priority_and_nice(int policy, int priority, int nice_value, int i) {
    struct sched_param param;
    param.sched_priority = priority;

    if (sched_setscheduler(0, policy, &param) == -1) {
        perror("sched_setscheduler failed");
        exit(1);
    }

    if (setpriority(PRIO_PROCESS, 0, nice_value) == -1) {
        perror("setpriority failed");
        exit(1);
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: %s <policy> <priority_level>\n", argv[0]);
        return 1;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    int policy;
    if (strcmp(argv[1], "SCHED_FIFO") == 0) {
        policy = SCHED_FIFO;
    } else if (strcmp(argv[1], "SCHED_RR") == 0) {
        policy = SCHED_RR;
    } else if (strcmp(argv[1], "SCHED_OTHER") == 0) {
        policy = SCHED_OTHER;
    } else {
        fprintf(stderr, "Invalid policy\n");
        return 1;
    }

    int max_priority = sched_get_priority_max(policy);
    int min_priority = sched_get_priority_min(policy);
    int priority;
    int nice_value;

    if (strcmp(argv[2], "highest") == 0) {
        priority = max_priority;
        nice_value = -20;
    } else if (strcmp(argv[2], "default") == 0) {
        priority = (max_priority + min_priority) / 2;
        nice_value = 0;
    } else if (strcmp(argv[2], "lowest") == 0) {
        priority = min_priority;
        nice_value = 19;
    } else {
        fprintf(stderr, "Invalid priority level\n");
        return 1;
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            set_priority_and_nice(policy, priority, nice_value, i);
        }
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        wait(NULL);
    }

    printf("Policy: %s, Priority: %d, Nice: %d\n", argv[1], priority, nice_value);

    gettimeofday(&end, NULL);

    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;

    if (micros < 0) {
        micros += 1000000;
        seconds--;
    }

    printf("Elapsed time: %ld.%06ld seconds\n", seconds, micros);

    return 0;
}
