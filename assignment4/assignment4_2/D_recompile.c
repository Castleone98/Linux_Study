#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include <linux/kernel.h>
#include <sys/user.h>

#define SHARED_MEM_SIZE PAGE_SIZE

uint8_t* source_code;
uint8_t* exec_code;

void sharedmem_init();
void sharedmem_exit();
void drecompile_init();
void drecompile_exit();
void* drecompile(uint8_t *func);
void optimize_code(uint8_t *code, size_t size);

int main(void)
{
   int (*func)(int a);
   struct timeval start, end;
   double elapsed_time;

   

   sharedmem_init();
   drecompile_init();

   func = (int (*)(int a))drecompile(source_code);

    


   gettimeofday(&start, NULL);
   int result = func(1);
   printf("result: %d\n", result);

   drecompile_exit();
   sharedmem_exit();

   gettimeofday(&end, NULL);

   elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0;
   elapsed_time += (end.tv_usec - start.tv_usec) / 1000.0;
   printf("Total execution time: %f ms\n", elapsed_time);

   return 0;
}

void sharedmem_init()
{
   int mem_segment_id;

   mem_segment_id = shmget(1234, SHARED_MEM_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR);

   if (mem_segment_id == -1) {
      perror("shmget");
      exit(EXIT_FAILURE);
   }

   source_code = (uint8_t*)shmat(mem_segment_id, NULL, 0);

   if (source_code == (void*)-1) {
      perror("shmat");
      exit(EXIT_FAILURE);
   }
}

void sharedmem_exit()
{
   if(shmdt(source_code) == -1) {
      perror("shmdt");
      exit(EXIT_FAILURE);
   }
}

void drecompile_init()
{
   int protection = PROT_READ | PROT_WRITE;
   int flags = MAP_PRIVATE | MAP_ANONYMOUS;

   exec_code = mmap(NULL, SHARED_MEM_SIZE, protection, flags, -1, 0);

   if (exec_code == MAP_FAILED) {
      perror("mmap");
      exit(EXIT_FAILURE);
   }
}

void drecompile_exit()
{
   if (munmap(exec_code, SHARED_MEM_SIZE) == -1) {
      perror("munmap");
      exit(EXIT_FAILURE);
   }
}

void* drecompile(uint8_t* func)
{
   int index = 0;

   do {
      exec_code[index++] = *func;
   } while(*func++ != 0xc3);

#ifdef dynamic
   optimize_code(exec_code, index);
#endif

   if (mprotect(exec_code, SHARED_MEM_SIZE, PROT_READ | PROT_EXEC) == -1) {
      perror("mprotect");
      exit(EXIT_FAILURE);
   }

   return exec_code;
}
void remove_nops_and_shift(uint8_t* code, size_t size) {
    size_t write_index = 0; // 쓰기 위치 인덱스
    for (size_t read_index = 0; read_index < size; ++read_index) {
        if (code[read_index] != 0x90) {
            // 현재 읽은 값이 NOP(0x90)이 아니라면, write_index 위치에 값을 씁니다.
            code[write_index] = code[read_index];
            ++write_index;
        }
    }

    // 남은 부분을 0으로 채웁니다.
    for (size_t i = write_index; i < size; ++i) {
        code[i] = 0x00;
    }
}

void update_last_op(uint8_t* code, uint8_t last_op, size_t last_op_index, int* total_add, int* total_sub, int* total_imul, int* div_count,int i) {
    // 대표 명령어 업데이트
    if (last_op == 0x83 && code[last_op_index + 1] == 0xc0) { // add
        memset(code + last_op_index, 0x90, i - last_op_index-3);  // 이전 add 명령어들을 NOP으로 대체
        code[i-1] = (uint8_t)(*total_add);
       
        *total_add = 0;
        // 디버깅: add 명령어 업데이트 정보 출력
        
    } else if (last_op == 0x83 && code[last_op_index + 1] == 0xe8) { // sub
        memset(code + last_op_index, 0x90, i - last_op_index-3);  // 이전 sub 명령어들을 NOP으로 대체
        code[i - 1] = (uint8_t)(*total_sub);
        //printf("Updated sub instruction: index=%zu, new_value=%d\n", last_op_index, *total_sub);
        //printf("test");
        *total_sub = 0;
        // 디버깅: sub 명령어 업데이트 정보 출력
        
    } else if (last_op == 0x6b && code[last_op_index + 1] == 0xc0) { // imul
        
        memset(code + last_op_index, 0x90, i - last_op_index-3);  // 이전 imul 명령어들을 NOP으로 대체
        code[i - 1] = (uint8_t)(*total_imul); // 새로운 값으로 바꿉니다
       // printf("Updated imul instruction: index=%zu, new_value=%d\n", last_op_index, *total_imul);
        *total_imul = 1;
        // 디버깅: imul 명령어 업데이트 정보 출력
        
    }
}




void optimize_code(uint8_t* code, size_t size) {
    int total_add = 0, total_sub = 0, total_imul = 1, div_count = 0;
    size_t i = 0, last_op_index = 0;
    uint8_t last_op = 0;
    uint8_t last_op_plus = 0;
    int flags =0;
    size_t last_used_index=0;
    
    while (i < size) {
        uint8_t op = code[i];
        uint8_t op_plus=code[i+1];
        uint8_t next_op = (i + 1 < size) ? code[i + 1] : 0;

        if(((op == 0x83 && next_op == 0xc0) || (op == 0x83 && next_op == 0xe8) || (op == 0x6b && next_op == 0xc0) || (op == 0xf6 && next_op == 0xf2)) &&(flags==0)){
            last_op=op;
            last_op_plus=op_plus;
            last_op_index=i;
            last_used_index=0;
            flags=1;
        }


        // 디버깅: 현재 처리 중인 명령어 정보 출력
        //printf("Processing instruction at index %zu: op=%02x, next_op=%02x\n", i, op, next_op);

        // add 명령어 처리
        if (op == 0x83 && next_op == 0xc0) {
            total_add += (int8_t)code[i + 2];
            if ((last_op == op && last_op_plus != op_plus) || (last_op != op && last_op_plus == op_plus)) {
                if (last_op != 0) {
                    
                    // 디버깅: update_last_op 함수 호출 직전 함수 인자 정보 출력
                  //  printf("Before update_last_op (add): last_op=%02x, last_op_index=%zu, total_add=%d, total_sub=%d, total_imul=%d, div_count=%d\n",
                   //        last_op, last_op_index, total_add, total_sub, total_imul, div_count);
                    update_last_op(code, last_op, last_op_index, &total_add, &total_sub, &total_imul, &div_count,i-last_used_index);
                    last_used_index=0;
                }
                last_op = op;
                last_op_plus =op_plus;
                last_op_index = i;
                // 디버깅: add 연산 시작
               // printf("Starting add operation: last_op=%02x, last_op_index=%zu, last_op_plus=%02x\n", last_op, last_op_index,last_op_plus);
            }
            i += 3;
            // 디버깅: add 명령어 처리 정보 출력
            //printf("Processed add instruction: total_add=%d\n", total_add);
        }
        // sub 명령어 처리
        else if (op == 0x83 && next_op == 0xe8) {
            total_sub += (int8_t)code[i + 2];
            if (last_op_plus != op_plus) {
                if (last_op != 0) {
                    // 디버깅: update_last_op 함수 호출 직전 함수 인자 정보 출력
               //     printf("Before update_last_op (sub): last_op=%02x, last_op_index=%zu, total_add=%d, total_sub=%d, total_imul=%d, div_count=%d\n",
                 //          last_op, last_op_index, total_add, total_sub, total_imul, div_count);
                    update_last_op(code, last_op, last_op_index, &total_add, &total_sub, &total_imul, &div_count,i-last_used_index);
                    last_used_index=0;
                }
                last_op = op;
                last_op_plus =op_plus;
                last_op_index = i;
                // 디버깅: sub 연산 시작
               // printf("Starting sub operation: last_op=%02x, last_op_index=%zu, last_op_plus=%02x\n", last_op, last_op_index,last_op_plus);
            }
            i += 3;
            // 디버깅: sub 명령어 처리 정보 출력
          //  printf("Processed sub instruction: total_sub=%d\n", total_sub);
        }
        // imul 명령어 처리
        else if (op == 0x6b && next_op == 0xc0) {
            total_imul *= (int8_t)code[i + 2];
            if (last_op != op) {
                if (last_op != 0) {
                    // 디버깅: update_last_op 함수 호출 직전 함수 인자 정보 출력
                 //   printf("Before update_last_op (imul): last_op=%02x, last_op_index=%zu, total_add=%d, total_sub=%d, total_imul=%d, div_count=%d\n",
                //           last_op, last_op_index, total_add, total_sub, total_imul, div_count);
                    update_last_op(code, last_op, last_op_index, &total_add, &total_sub, &total_imul, &div_count,i-last_used_index);
                    last_used_index=0;
                }
                last_op = op;
                last_op_plus =op_plus;
                last_op_index = i;
                // 디버깅: imul 연산 시작
             //   printf("Starting imul operation: last_op=%02x, last_op_index=%zu, last_op_plus=%02x\n", last_op, last_op_index,last_op_plus);
            }
            i += 3;
            // 디버깅: imul 명령어 처리 정보 출력
         //   printf("Processed imul instruction: total_imul=%d\n", total_imul);
        }
        else {
            i++;
            last_used_index++;
        }
        //printf("size: %zu  i: %zu", size, i);
        // 마지막 명령어 처리
        if (i == size && last_op != 0) {
            
            // 디버깅: update_last_op 함수 호출 직전 함수 인자 정보 출력
            //printf("Before update_last_op (final): last_op=%02x, last_op_index=%zu, total_add=%d, total_sub=%d, total_imul=%d, div_count=%d\n",
            //       last_op, last_op_index, total_add, total_sub, total_imul, div_count);
            update_last_op(code, last_op, last_op_index, &total_add, &total_sub, &total_imul, &div_count,i-last_used_index);
            
            remove_nops_and_shift(code,size);
            
            printf("Dynamic Enabled\n");
        }
    }
}



