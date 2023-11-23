#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h> 

#define MAX_INPUT_SIZE 1024

void removeDirectory(const char *path, bool i, bool v, bool f) {
    DIR *dir;
    struct dirent *entry;
    char fullpath[MAX_INPUT_SIZE];

    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    // 디렉토리와 그 안의 모든 내용물을 삭제
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            struct stat file_stat;
            if (lstat(fullpath, &file_stat) == -1) {
                perror("Error getting file status");
                continue;
            }

            if (S_ISDIR(file_stat.st_mode)) {
                // 디렉토리일 때 재귀적으로 호출
                removeDirectory(fullpath, i, v, f);
            } else {
                // 파일일 때 삭제
                if (i) {
                    char answer[MAX_INPUT_SIZE];
                    printf("delete the file '%s'? (y/n): ", fullpath);
                    // printf("파일 '%s'을(를) 삭제하시겠습니까? (y/n): ", fullpath);
                    fgets(answer, sizeof(answer), stdin);

                    // 개행 문자를 제거
                    answer[strcspn(answer, "\n")] = '\0';

                    if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
                        // printf("파일 '%s' 삭제가 취소되었습니다.\n", fullpath);
                        continue; // 'n'이 입력되면 현재 파일을 건너뛰고 다음 파일로 이동
                    } else if (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0) {
                        // printf("잘못된 입력입니다. 'y' 또는 'n'을 입력하세요.\n");
                        printf("Invalid input. Please enter 'y' or 'n'.\n");
                        continue; // 'y'나 'n'이 아닌 경우 다음 파일로 이동
                    }
                }

                // 파일을 삭제
                if (remove(fullpath) != 0) {
                    // perror("파일 삭제 중 오류 발생");
                    perror("Error removing file");
                } else {
                    if (v) {
                        // printf("파일 '%s'을(를) 삭제했습니다.\n", fullpath);
                        printf("removed file '%s'\n", fullpath);
                    }
                }
            }
        }
    }

    closedir(dir);

    if (i) {
        char answer[MAX_INPUT_SIZE];
        printf("delete the directory '%s'? (y/n): ", path);
        // printf("파일 '%s'을(를) 삭제하시겠습니까? (y/n): ", fullpath);
        fgets(answer, sizeof(answer), stdin);

        // 개행 문자를 제거
        answer[strcspn(answer, "\n")] = '\0';

        if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
            // printf("파일 '%s' 삭제가 취소되었습니다.\n", fullpath);
            return; // 'n'이 입력되면 현재 파일을 건너뛰고 다음 파일로 이동
        } else if (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0) {
            // printf("잘못된 입력입니다. 'y' 또는 'n'을 입력하세요.\n");
            printf("Invalid input. Please enter 'y' or 'n'.\n");
            return; // 'y'나 'n'이 아닌 경우 다음 파일로 이동
        }
    }
    // 디렉토리를 삭제
    if (rmdir(path) != 0) {
        // perror("디렉토리 삭제 중 오류 발생");
        perror("Error removing Directory");
    } else {
        if (v) {
            // printf("디렉토리 '%s'을(를) 삭제했습니다.\n", path);
            printf("removed Directory '%s'\n", path);
        }
    }
}


void remove_file(char *tokens[]) {
    // 옵션 파싱
    bool i = false, r = false, v = false, f = false;

    int s = 1;

    if (tokens[s] != NULL && strchr(tokens[s], '-') != NULL && tokens[s][1] != '\0') {
        if (strchr(tokens[s], 'i') != NULL)
            i = true;
        if (strchr(tokens[s], 'r') != NULL)
            r = true;
        if (strchr(tokens[s], 'v') != NULL)
            v = true;
        if (strchr(tokens[s], 'f') != NULL)
            f = true;
        s = 2;   
    }
    if (f == true && i == true)
        i == false;
    
    for (int k = s; tokens[k] != NULL; k++) {
        struct stat file_stat;

        if (stat(tokens[k], &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                // 디렉토리일 때의 처리
                if (r) {
                    removeDirectory(tokens[k], i, v, f);
                } else {
                    // 에러 처리: 디렉토리를 삭제하려는데 -r 옵션이 주어지지 않은 경우
                    fprintf(stderr, "rm: cannot remove '%s': Is a directory\n", tokens[k]);
                }
            } else if (S_ISREG(file_stat.st_mode)) {
                if (i){
                    char answer[MAX_INPUT_SIZE];
                    printf("delete the file? (y/n): ");
                    fgets(answer, sizeof(answer), stdin);

                    // 개행 문자를 제거
                    answer[strcspn(answer, "\n")] = '\0';

                    if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
                        continue; // 'n'이 입력되면 현재 파일을 건너뛰고 다음 파일로 이동
                    } else if (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0) { 
                        printf("Invalid input. Please enter 'y' or 'n'.\n");
                        continue; // 'y'나 'n'이 아닌 경우 다음 파일로 이동
                    }
                }
                // 일반 파일일 때의 처리
                // v: 파일 삭제 메시지 출력
                if (v) {
                    printf("removed file '%s'\n", tokens[k]);
                }

                // 파일을 삭제
                if (remove(tokens[k]) != 0) {
                    if (!f)
                        perror("Error removing file");
                }
            } else {
                // 에러 처리: 파일도 아니고 디렉토리도 아닌 경우
                perror("Error removing file");
            }
        } else {
            if (!f)
                // 에러 처리: 파일 정보를 가져오는데 실패한 경우
                perror("Error getting file information");
        }
    }
}

int main(int argc, char *argv[]){
    if (argc == 1){
        printf("사용법: rm <파일1> <파일2> ...\n");
        printf("파일을 삭제합니다.\n");
        printf("\n옵션:\n");
        printf("  -f          존재하지 않는 파일 및 인자를 무시하고 물어보지 않음\n");
        printf("  -i          각 파일을 삭제하기 전에 확인 메시지 출력\n");
        printf("  -r          디렉토리 및 그 내용물을 재귀적으로 삭제\n");
        printf("  -v          삭제된 파일 및 디렉토리를 출력\n");
        return 0;
    }
    remove_file(argv);
    return 0;
}