#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h> 
#include <pwd.h>   
#include <grp.h>  


void link_file(char *tokens[]) {
   
    bool s = false, f = false;  // 옵션 플래그
    char *source = NULL, *target = NULL;  // 소스 및 대상 파일 경로 초기화

    for (int i = 1; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "-s") == 0) {
            s = true;  // 심볼릭 링크 옵션 활성화
        } else if (strcmp(tokens[i], "-f") == 0) {
            f = true;  // 하드 링크 옵션 활성화
        } else if (source == NULL) {
            source = tokens[i]; // 첫번째 파일 경로
        } else {
            target = tokens[i]; // 두번째 파일 경로
        }
    }

    // 소스 또는 대상이 지정되지 않은 경우
    if (source == NULL || target == NULL) {
        fprintf(stderr, "Usage: ln [-s] [-f] source target\n");
        return;
    }

    // 심볼릭 링크 생성 로직
    if (s) {
        // -f 옵션이 있고 대상 파일이 존재하는 경우, 기존 링크를 삭제
        if (f && access(target, F_OK) == 0) {
            if (unlink(target) < 0) {
                perror("Failed to remove existing file");
                return;
            }
        }
        if (symlink(source, target) < 0) {
            perror("Failed to create symbolic link");
        } else {
            printf("Symbolic link created: %s -> %s\n", target, source);
        }
    } else {
        // 하드 링크 생성
        if (f && access(target, F_OK) == 0) {
            if (unlink(target) < 0) {
                perror("Failed to remove existing file");
                return;
            }
        }
        if (link(source, target) < 0) {
            perror("Failed to create hard link");
        } else {
            printf("Hard link created: %s -> %s\n", target, source);
        }
    }
}

int main(int argc, char *argv[]) {

    if (argc == 1){
        printf("사용법: ln <파일1> <파일2>\n");
        printf("파일 간의 링크를 생성합니다.\n");
        printf("\n옵션:\n");
        printf("  -f:  접근할 수 없는 사용권한을 가졌을 때도 링크가 가능\n");
        printf("  -s:  심볼릭 링크 생성\n");
    }

    link_file(argv);
    return 0;

}