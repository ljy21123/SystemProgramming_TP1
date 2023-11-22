#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

void remove_directory(char *tokens[]) {
    // 디렉토리 삭제
    if (rmdir(tokens[1]) == 0) {
        printf("directory delete success!!: %s\n", tokens[1]);
    } else {
        perror("directory delete fail..");
    }
}

int main(int argc, char *argv[]) {

    if (argc == 1){
        printf("사용법: rmdir <폴더>\n");
        printf("폴더를 삭제합니다.\n");
    }

    remove_directory(argv);
    return 0;

}