#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <sys/time.h>  // gettimeofday를 위한 헤더
#define MAX_PROCESSES 64

typedef struct {
    int *nums;
    int start;
    int end;
} ThreadArgs;

void *recursive_sum(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int *nums = args->nums;
    int start = args->start;
    int end = args->end;

    if (end - start == 1) {
        long *result = malloc(sizeof(long));
        *result = nums[start];
        pthread_exit(result);
    } else if (end - start == 0) {
        long *result = malloc(sizeof(long));
        *result = 0;
        pthread_exit(result);
    }

    int mid = (start + end) / 2;

    pthread_t tid1, tid2;

    ThreadArgs args1 = {nums, start, mid};
    ThreadArgs args2 = {nums, mid, end};

    long *left_sum, *right_sum;

    pthread_create(&tid1, NULL, recursive_sum, &args1);
    pthread_create(&tid2, NULL, recursive_sum, &args2);

    pthread_join(tid1, (void **)&left_sum);
    pthread_join(tid2, (void **)&right_sum);

    long *total_sum = malloc(sizeof(long));
    *total_sum = *left_sum + *right_sum;

    free(left_sum);
    free(right_sum);

    pthread_exit(total_sum);
}

int main() {
    pthread_t tid;

    int nums[MAX_PROCESSES * 2];
    FILE *f_read = fopen("temp.txt", "r");
    int count = 0;
    while (fscanf(f_read, "%d", &nums[count]) != EOF) {
        count++;
    }
    fclose(f_read);

    ThreadArgs args = {nums, 0, count};

    struct timeval start, end;
    gettimeofday(&start, NULL);  // 시작 시간 캡쳐

    pthread_create(&tid, NULL, recursive_sum, &args);

    long *sum;
    pthread_join(tid, (void **)&sum);

    gettimeofday(&end, NULL);  // 종료 시간 캡쳐

    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_usec - start.tv_usec) / 1e6;  // 실행 시간 계산

    printf("value from thread: %ld\n", *sum);
    printf("%f\n", elapsed_time);  // 실행 시간 출력

    free(sum);

    return 0;
}