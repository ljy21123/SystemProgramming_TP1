/*
    명령어 형식은 input에 저장된 후 '\n'(엔터)를 제거하고 마지막에 '\0'를 추가하여 입력을 마치고
    공백을 기준으로 나누어 tokens에 저장됩니다.
    예) mv -v a.txt test_dir/ => ["mv", "-v", "a.txt", "test_dir/"] 형식으로 변환
    tokens[0] => mv 출력됨.

    2023-11-11: exit, 파일재지향, 파이프, rm, mv 기능 생성,
                다른 코드에 정의된 mkdir과 통합

*/

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

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

void remove_file(char *tokens[]);
void move_file(char *tokens[]);
void handle_redirection(char *tokens[]);
void mkdir_method(/*char *tokens[]*/char *pathname);

int main(int argc, char *argv[]) {
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];
    bool pipe = false;

    while (1) {
        // 프롬프트 표시
        printf("MyShell> ");
        // 출력이 즉시 화면에 나타나도록 함
        fflush(stdout);

        // 사용자 입력 받기
        fgets(input, MAX_INPUT_SIZE, stdin);

        // 엔터 키만 입력한 경우 루프 다시 시작
        if (strlen(input) == 0 || input[0] == '\n') {
            continue;
        }

        // 개행 문자 제거하고 명령어의 끝을 표기
        input[strcspn(input, "\n")] = '\0';

        // 토큰화
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            tokens[i++] = token;
            token = strtok(NULL, " ");
        }
        tokens[i] = NULL;

        pipe = false;
        for (i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "|") == 0) {
                pipe = true;
            }
        }


        // exit 명령어 처리
        if (strcmp(tokens[0], "exit") == 0) {
            break;
        }
        // 파이프 처리
        else if (pipe){
            handle_redirection(tokens);
        }
        // mv 명령어 처리
        else if (strcmp(tokens[0], "mv") == 0) {
            move_file(tokens);
        }
        // rm 명령어 처리
        else if (strcmp(tokens[0], "rm") == 0) {
            remove_file(tokens);
        }
        // mkdir 명령어 처리
        else if(strcmp(tokens[0], "mkdir") == 0) {
            mkdir_method(tokens[1]);
        }
        else{
            printf("command not found...\n");
        }
    }
    return 0;
}

void mkdir_method(/*char *tokens[]*/char *pathname) {
    // char *pathname = tokens[1];
    // 디렉토리 생성
    if (mkdir(pathname, S_IRWXU) == 0) {
        printf("directory create success!!: %s\n", pathname);
    } else {
        perror("directory create fail..");
    }
}

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
                printf("renamed the file? (y/n): ");
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

void remove_file(char *tokens[]) {
    // 옵션 파싱
    bool i = false, r = false, v = false;

    int s = 1;

    if (tokens[s] != NULL && strchr(tokens[s], '-') != NULL && tokens[s][1] != '\0') {
        if (strchr(tokens[s], 'i') != NULL)
            i = true;
        if (strchr(tokens[s], 'r') != NULL)
            r = true;
        if (strchr(tokens[s], 'v') != NULL)
            v = true;
        s = 2;   
    }

    for (int k = s; tokens[k] != NULL; k++) {
        struct stat file_stat;

        if (stat(tokens[k], &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                // 디렉토리일 때의 처리
                if (r) {
                    // -r 옵션이 주어진 경우에만 디렉토리를 삭제
                    // i: 파일을 삭제할지 물어봄
                    if (i) {
                        char answer[MAX_INPUT_SIZE];
                        printf("delete the directory? (y/n): ");
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

                    if (v) {
                        printf("removed directory '%s'\n", tokens[k]);
                    }

                    // 디렉토리를 삭제
                    if (remove(tokens[k]) != 0) {
                        perror("Error removing directory");
                    }
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
                    perror("Error removing file");
                }
            } else {
                // 에러 처리: 파일도 아니고 디렉토리도 아닌 경우
                perror("Error removing file");
            }
        } else {
            // 에러 처리: 파일 정보를 가져오는데 실패한 경우
            perror("Error getting file information");
        }
    }
}

void handle_redirection(char *tokens[]) {
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);

    int input_redirection = -1;
    int output_redirection = -1;
    int pipe_position = -1;

    // Find redirection symbols and their positions
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            input_redirection = i;
        } else if (strcmp(tokens[i], ">") == 0) {
            output_redirection = i;
        } else if (strcmp(tokens[i], "|") == 0) {
            pipe_position = i;
        }
    }

    if (pipe_position != -1) {
        // 파이프가 존재하는 경우
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            return;
        }

        // 왼쪽 명령어 실행
        pid_t pid_left = fork();
        if (pid_left == 0) {
            close(pipe_fd[0]); // 파이프 읽기 닫기
            dup2(pipe_fd[1], 1); // 표준 출력을 파이프 쓰기로 변경
            close(pipe_fd[1]); // 기존 표준 출력 닫기

            // 실행할 명령어 설정 (tokens 배열을 적절히 자름)
            execvp(tokens[0], tokens);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid_left < 0) {
            perror("fork");
            return;
        }

        // 오른쪽 명령어 실행
        pid_t pid_right = fork();
        if (pid_right == 0) {
            close(pipe_fd[1]); // 파이프 쓰기 닫기
            dup2(pipe_fd[0], 0); // 표준 입력을 파이프 읽기로 변경
            close(pipe_fd[0]); // 기존 표준 입력 닫기

            // 실행할 명령어 설정 (tokens 배열을 적절히 자름)
            execvp(tokens[pipe_position + 1], tokens + pipe_position + 1);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid_right < 0) {
            perror("fork");
            return;
        }

        // 부모 프로세스에서는 파이프 닫기 및 자식 프로세스의 종료를 기다림
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        waitpid(pid_left, NULL, 0);
        waitpid(pid_right, NULL, 0);
    } else {
        // 그 외의 경우는 기존의 리다이렉션 코드를 수행
        if (input_redirection != -1) {
            // 입력 리다이렉션 처리
            int fd = open(tokens[input_redirection + 1], O_RDONLY);
            if (fd < 0) {
                perror("open");
                return;
            }
            dup2(fd, 0);
            close(fd);
            // Remove redirection tokens from the command
            tokens[input_redirection] = NULL;
            tokens[input_redirection + 1] = NULL;
        }

        if (output_redirection != -1) {
            // 출력 리다이렉션 처리
            int fd = open(tokens[output_redirection + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                perror("open");
                return;
            }
            dup2(fd, 1);
            close(fd);
            // Remove redirection tokens from the command
            tokens[output_redirection] = NULL;
            tokens[output_redirection + 1] = NULL;
        }

        // 명령어 실행
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // 자식 프로세스
            execvp(tokens[0], tokens);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (child_pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            // 부모 프로세스에서 자식 프로세스의 종료를 기다림
            waitpid(child_pid, NULL, 0);
        }
    }

    // 리다이렉션이 적용된 프로세스에서 파일 디스크립터를 복원
    dup2(saved_stdout, 1);
    dup2(saved_stdin, 0);
    close(saved_stdout);
    close(saved_stdin);
}
