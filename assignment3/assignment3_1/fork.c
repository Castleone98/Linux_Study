#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>  // gettimeofday를 위한 헤더

#define MAX_PROCESSES 64


int recursive_sum(int *nums, int start, int end) {
    if (end - start == 1) {
       
        return nums[start];
    } else if (end - start == 0) {
        return 0;
    }

    int mid = (start + end) / 2;
    pid_t pid = fork();

    if (pid == 0) { // 자식 프로세스
        int left_sum = recursive_sum(nums, start, mid);
      //  if (left_sum > 255) {  // 8비트 값을 초과하는지 확인
      //      exit(1);  // 8비트 값을 초과하면 1을 반환하여 종료
       // }

        exit(left_sum%256);
    } else { // 부모 프로세스
        int right_sum = recursive_sum(nums, mid, end);
        right_sum=(right_sum%256);
        int status;
        wait(&status);
        int left_sum = (status>>8);

        //if (status != 0) {  // 자식 프로세스가 1을 반환한 경우
        //    return MAX_PROCESSES;  // MAX_PROCESSES 값을 반환
      //  }
        
        return left_sum + right_sum;
    }
}



int main() {
    int nums[MAX_PROCESSES * 2];
    FILE *f_read = fopen("temp.txt", "r");
    int count = 0;
    while (fscanf(f_read, "%d", &nums[count]) != EOF) {
        count++;
    }
    fclose(f_read);

    struct timeval start, end;
    gettimeofday(&start, NULL);  // 시작 시간 캡쳐

    int total_sum = recursive_sum(nums, 0, count);

    gettimeofday(&end, NULL);  // 종료 시간 캡쳐

    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_usec - start.tv_usec) / 1e6;  // 실행 시간 계산

    printf("value of fork: %d\n", total_sum);
    printf("%f\n", elapsed_time);  // 실행 시간 출력

    return 0;
}
