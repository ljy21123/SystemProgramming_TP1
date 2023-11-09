#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    char command[256];

    while (1) {
        printf("shell> ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        // 줄 끝의 개행 문자 제거
        command[strcspn(command, "\n")] = 0;

        // mkdir 명령어 확인
        if (strncmp(command, "mkdir ", 6) == 0) {
            printf("test success!\n");
        } else {
            printf("%s: command not found\n", command);
        }
    }

    return 0;
}
