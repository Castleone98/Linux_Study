#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PROCESSES 10000

int main() {
    // Step 1: "temp" 디렉토리 생성
    mkdir("temp", 0700);
    
    // 난수 생성을 위한 시드 설정
    srand(time(NULL));
    
    // Step 2: 무작위의 integer형 양수가 기록된 파일 생성
    for(int i = 0; i < MAX_PROCESSES; i++) {
        char filename[256];
        sprintf(filename, "./temp/%d", i);
        
        FILE *f_write = fopen(filename, "w");
        if(f_write == NULL) {
            printf("파일 생성에 실패했습니다: %s\n", filename);
            return 1;
        }
        
        // 1부터 9까지의 무작위 정수를 파일에 기록
        fprintf(f_write, "%d", 1 + rand() % 9);
        fclose(f_write);
    }

    return 0;
}
