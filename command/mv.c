#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define MAX_INPUT_SIZE 1024

void move_file(char *tokens[]) {
    // 옵션 파싱
    /*
    -b : 이동 위치에 동일한 이름의 파일이 있는 경우 이동 위치의 파일의 이름에 ~를 붙여 이름을 바꾼후 이동
    -i : 대상 파일이 기존 파일이면, 덮어쓸 것인지 물어봄
    -u : 대상 파일보다 원본 파일이 최근의 것일 때 업그레이드
    -v : 파일 옮기는 과정을 자세하게 보여준다
    */
    // 토큰의 개수
    int token_count = 0;
    while (tokens[token_count] != NULL) {
        token_count++;
    }
    token_count--; // 마지막 타겟은 제외

    bool b = false, i = false, u = false, v = false;

    int s = 1;
    if (tokens[s] != NULL && strchr(tokens[s], '-') != NULL && tokens[s][1] != '\0'){ 
        if (strchr(tokens[s], 'b') != NULL)
            b = true;
        if (strchr(tokens[s], 'i') != NULL)
            i = true;
        if (strchr(tokens[s], 'u') != NULL)
            u = true;
        if (strchr(tokens[s], 'v') != NULL)
            v = true;
        s = 2;
    }

    char *target;
    target = (char *)calloc(strlen(tokens[s + 1]) + 1, sizeof(char));
    strcpy(target, tokens[token_count]);

    if(token_count - s > 1){
        struct stat target_stat;
        if (lstat(target, &target_stat) == 0) {
            if (!S_ISDIR(target_stat.st_mode)) {
                fprintf(stderr, "target '%s' is not a directory\n", target);
                return; 
            }
        } else {
            perror("Error getting file information");
            return;
        }
    }

    for (; s < token_count; s++){
        struct stat buf;
        
        char *src_file_name_only;

        if (tokens[s] == NULL || tokens[token_count] == NULL) {
            fprintf(stderr, "Usage: move_file src target\n");
            return;
        }

        if (access(tokens[s], F_OK) < 0) {
            fprintf(stderr, "%s not exists\n", tokens[s]);
            return;
        } else {
            char *slash = strrchr(tokens[s], '/');
            src_file_name_only = tokens[s];
            if (slash != NULL) { 
                src_file_name_only = slash + 1;
            }
        }


        if (access(tokens[token_count], F_OK) == 0) {
            if (lstat(tokens[token_count], &buf) < 0) {
                perror("lstat");
                free(target);
                return;
            } else {
                if (S_ISDIR(buf.st_mode)) {
                    free(target);
                    target = (char *)calloc(strlen(tokens[s]) + strlen(tokens[token_count]) + 2, sizeof(char));
                    strcpy(target, tokens[token_count]);
                    strcat(target, "/");
                    strcat(target, src_file_name_only);
                }
            }
        }

        if (i){
            // 타겟 경로에 파일이 존재한다면
            if (access(target, F_OK) == 0) {
                char answer[MAX_INPUT_SIZE];
                printf("overwrite '%s/%s'? (y/n): ", tokens[3], tokens[2]);
                fgets(answer, sizeof(answer), stdin);

                // 개행 문자를 제거
                answer[strcspn(answer, "\n")] = '\0';

                if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
                    return; // 'n'이 입력되면 현재 파일을 건너뛰고 다음 파일로 이동
                } else if (strcmp(answer, "y") != 0 && strcmp(answer, "Y") != 0) { 
                    printf("Invalid input. Please enter 'y' or 'n'.\n");
                    return; // 'y'나 'n'이 아닌 경우 다음 파일로 이동
                }
            }
        }

        if (b){
            if (access(target, F_OK) == 0) {
                char new_target[256];
                snprintf(new_target, sizeof(new_target), "%s~", target);

                char *command[] = {"mv", target, new_target, NULL};
                move_file(command);
            }
        }

        if (u) {
            struct stat source_stat, dest_stat;

            if (lstat(tokens[s], &source_stat) == 0 && lstat(target, &dest_stat) == 0) {
                // lstat 함수를 사용하여 파일의 메타데이터를 가져옴

                // 비교해서 source가 더 최신이면 업데이트
                if (difftime(source_stat.st_mtime, dest_stat.st_mtime) > 0) {
                    if (rename(tokens[s], target) < 0) {
                        perror("rename");
                        return;
                    }
                } else {
                    printf("'%s' is not newer than...\n", tokens[s]);
                    free(target);
                    return;
                }
            } else {
                perror("Error getting file information");
                free(target);
                return;
            }
        }
        else{
            if (rename(tokens[s], target) < 0) {
                perror("rename");
                return;
            }
        }

        if (v){
            printf("renamed '%s' -> '%s'\n", tokens[s], target);
        }

        
    }
    free(target);
}

int main(int argc, char *argv[]){
    if (argc == 1){
        printf("사용법: mv <원본 경로> <대상 경로>\n");
        printf("파일 또는 디렉토리를 이동하거나 이름을 변경합니다.\n");
        printf("\n옵션:\n");
        printf("  -b, --backup          각 대상 파일에 대해 백업을 만듭니다\n");
        printf("  -i, --interactive     파일을 덮어쓰기 전에 확인 메시지를 출력합니다\n");
        printf("  -u, --update          대상 파일이 원본 파일보다 최신인 경우에만 이동합니다\n");
        printf("  -v, --verbose         수행 중인 작업을 자세히 설명합니다\n");
        return 0;
    }

    move_file(argv);
    return 0;
}