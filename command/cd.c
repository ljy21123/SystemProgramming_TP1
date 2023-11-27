#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void change_directory(const char *path) {
    if (chdir(path) != 0) {
        perror("chdir");
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("사용법: cd <path>\n");
    }
    change_directory(argv[1]);
    return 0;
}