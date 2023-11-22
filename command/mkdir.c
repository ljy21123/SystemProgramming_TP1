#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>

void make_directory(char *tokens[]) {
    // 디렉토리 생성
    if (mkdir(tokens[1], S_IRWXU) == 0) {
        printf("directory create success!!: %s\n", tokens[1]);
    } else {
        perror("directory create fail..");
    }
}

int main(int argc, char *argv[]) {

    if (argc == 1){
        printf("사용법: mkdir <폴더>\n");
        printf("새로운 폴더를 생성합니다.\n");
    }

    make_directory(argv);
    return 0;

}