#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


void mkdir_method();

int main() {
    char command[256];

    while (1) {
        printf("shell> ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        // 줄 끝의 개행 문자 제거
        command[strcspn(command, "\n")] = 0;
        // 명령어와 디렉토리 이름 분리
        char *token = strtok(command, " ");


        /* mkdir */
        if (token != NULL && strcmp(token, "mkdir") == 0) { // 명령어 확인
            char *dir_name = strtok(NULL, " "); // 디렉토리 이름 확인
            if (dir_name != NULL) {
                mkdir_method(dir_name);
            } else {
                printf("Please enter a folder name.\n");
            }

        // if (strncmp(command, "mkdir ", 6) == 0) {
        //     const char *mkdir_name = "create_test";
        //     mkdir_method(mkdir_name);
        //     printf("test success!\n");

        } else {
            printf("%s: command not found.\n", command);
        }


    }

    return 0;
}


void mkdir_method(const char *pathname) {

    // 디렉토리 생성
    int status = mkdir(pathname, S_IRWXU);

    if (status == 0) {
        printf("directory create success!!: %s\n", pathname);
    } else {
        perror("directory create fail..");
    }
}
