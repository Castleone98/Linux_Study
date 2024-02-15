#include <stdio.h>

#define MAX_PROCESSES 64

int main() {
    FILE *f_write = fopen("temp.txt", "w");
    for (int i = 0; i < MAX_PROCESSES * 2; i++) {
        fprintf(f_write, "%d\n", i + 1);
    }
    fclose(f_write);
    return 0;
}
