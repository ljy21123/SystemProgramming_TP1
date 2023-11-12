/*
    명령어 형식은 input에 저장된 후 '\n'(엔터)를 제거하고 마지막에 '\0'를 추가하여 입력을 마치고
    공백을 기준으로 나누어 tokens에 저장됩니다.
    예) mv -v a.txt test_dir/ => ["mv", "-v", "a.txt", "test_dir/"] 형식으로 변환
    tokens[0] => mv 출력됨.

    2023-11-09: mkdir 기능 생성
    2023-11-11: exit, 파일재지향, 파이프, rm, mv 기능 생성,
                다른 코드에 정의된 mkdir과 통합,
                rmdir, ln, 백그라운드 실행 기능 생성,
    2023-11-12: rm 디렉토리 순회 삭제 구현,
                cp, cat 기능 생성
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
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h> 
#include <pwd.h>   
#include <grp.h>  

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

void remove_file(char *tokens[]);
void move_file(char *tokens[]);
void handle_redirection(char *tokens[]);
void make_directory(char *pathname);
void remove_directory(char *pathname);
void removeDirectory(const char *path, bool i, bool v);
void link_file(char *tokens[]);
void bg_run(char *tokens[], bool background);
void list_directory(char *tokens[]);
void print_current_directory(void);
void change_directory(const char *path);
void handle_interrupt(int signo);
void copy_file(const char *source, const char *destination, bool f, bool inter, bool p, bool r);
void print_special_char(char ch, bool v, bool e, bool t);
void concatenate(char *tokens[]);


int main(int argc, char *argv[]) {
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];
    bool pipe = false;
    bool background = false;
 
    // 인터럽트 처리, 시그널 핸들러 설정
    if (signal(SIGINT, handle_interrupt) == SIG_ERR || signal(SIGTSTP, handle_interrupt) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    while (1) {

        // 프롬프트 표시
        printf("MyShell> ");
        // 출력이 즉시 화면에 나타나도록 함
        fflush(stdout);

        background = false;

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
        // while (token != NULL) {
        //     tokens[i++] = token;
        //     token = strtok(NULL, " ");
        // }
        while (token != NULL) {
            // '&' 문자가 토큰의 끝에 있는지 확인
            if (token[strlen(token) - 1] == '&') {
                background = true;
                // '&' 문자 제거
                token[strlen(token) - 1] = '\0';
                // 빈 토큰인 경우 넘어감
                if (strlen(token) == 0) {
                    break;
                }
            }
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
        // 백그라운드 실행 명령어 처리
        else if (background) {
            bg_run(tokens, background);
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
            make_directory(tokens[1]);
        }
        // rmdir 명령어 처리
        else if(strcmp(tokens[0], "rmdir") == 0) {
            remove_directory(tokens[1]);
        }
        // ln 명령어 처리
        else if(strcmp(tokens[0], "ln") == 0) {
            link_file(tokens);
        }
        // ls 명령어 처리
        else if (strcmp(tokens[0], "ls") == 0) {
            list_directory(tokens);
        }
        // pwd 명령어 처리
        else if (strcmp(tokens[0], "pwd") == 0) {
            print_current_directory();
        }
        // cd 명령어 처리
        else if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL) {
                fprintf(stderr, "사용법: cd <디렉터리>\n");
            } else {
                change_directory(tokens[1]);
            }
        }

        // cp 명령어 처리
        else if(strcmp(tokens[0], "cp") == 0) {
            bool f = false, inter = false, p = false, r = false;
            char *source = NULL, *destination = NULL;

            // 옵션 파싱
            for (int i = 1; tokens[i] != NULL; i++) {
                if (strcmp(tokens[i], "-f") == 0) {
                    f = true;
                } else if (strcmp(tokens[i], "-i") == 0) {
                    inter = true;
                } else if (strcmp(tokens[i], "-p") == 0) {
                    p = true;
                } else if (strcmp(tokens[i], "-r") == 0) {
                    r = true;
                } else {
                    if (source == NULL) {
                        source = tokens[i];
                    } else {
                        destination = tokens[i];
                    }
                }
            }

            if (source != NULL && destination != NULL) {
                copy_file(source, destination, f, inter, p, r);
            } else {
                fprintf(stderr, "Usage: cp [-fipr] source destination\n");
            }
        }

        // cat 명령어 처리
        else if (strcmp(tokens[0], "cat") == 0) {
            concatenate(tokens);
        }
        else {
            printf("command not found...\n");
        }
        
    }
    return 0;
}

void make_directory(char *pathname) {
    // 디렉토리 생성
    if (mkdir(pathname, S_IRWXU) == 0) {
        printf("directory create success!!: %s\n", pathname);
    } else {
        perror("directory create fail..");
    }
}

void remove_directory(char *pathname) {
    // 디렉토리 삭제
    if (rmdir(pathname) == 0) {
        printf("directory delete success!!: %s\n", pathname);
    } else {
        perror("directory delete fail..");
    }
}

void link_file(char *tokens[]) {
    /*
    -f : 접근할 수 없는 사용권한을 가졌을 때도 링크가 가능
    -s : 심볼릭 링크 생성
    */
   
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

void bg_run(char *tokens[], bool background) {
    pid_t child_process = fork();
    if (child_process == 0) {
        // 자식 프로세스: 명령어 실행
        execvp(tokens[0], tokens);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (child_process < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else {
        // 백그라운드 실행인 경우
        if (background) {
            printf("Process running in background with PID: %d\n", child_process);
            // waitpid를 호출하지 않아 백그라운드 프로세스가 독립적으로 실행됨
        } else {
            // 부모 프로세스: 자식 프로세스 종료를 기다림
            waitpid(child_process, NULL, 0);
        }
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

void removeDirectory(const char *path, bool i, bool v) {
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
                removeDirectory(fullpath, i, v);
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
                    removeDirectory(tokens[k], i, v);
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

struct FileInfoByMTime {
    char name[256];
    time_t mtime;  
};
struct FileInfoBySize {
    char name[256];
    off_t size;  
};
struct FileInfo {
    char name[256];
};
int compareFileInfoByMTime(const void *a, const void *b) {
    return ((struct FileInfoByMTime *)b)->mtime - ((struct FileInfoByMTime *)a)->mtime;
}
int compareFileInfoBySize(const void *a, const void *b) {
    return ((struct FileInfoBySize *)b)->size - ((struct FileInfoBySize *)a)->size;
}
int compareFileInfoByNameReverse(const void *a, const void *b) {
    return strcmp(((struct FileInfo *)b)->name, ((struct FileInfo *)a)->name);
}

void list_directory(char *tokens[]) {

    // 기본 ls 실행
    if (tokens[1] == NULL) {
        DIR *dir;
        struct dirent *entry;

        if ((dir = opendir(".")) != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                printf("%s\n", entry->d_name);
            }
            closedir(dir);
        } else {
            perror("디렉터리 열기 오류");
        }
    }
    // ls 뒤에 옵션이 있는 경우 처리
    else {
        for (int i = 1; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "-a") == 0) {
                DIR *dir;
                struct dirent *entry;

                if ((dir = opendir(".")) != NULL) {
                    // 디렉토리 내의 모든 파일과 디렉토리 출력
                    while ((entry = readdir(dir)) != NULL) {
                        printf("%s\n", entry->d_name);
                    }
                    closedir(dir);
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-l") == 0) {
                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    // 파일 및 디렉토리에 대한 상세 정보 출력
                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            struct stat file_stat;
                            char full_path[1024];
                            snprintf(full_path, sizeof(full_path), "%s/%s", ".", ent->d_name);

                            if (stat(full_path, &file_stat) == 0) {
                                // 파일 유형과 권한 출력
                                printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
                                printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
                                printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
                                printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
                                printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
                                printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
                                printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
                                printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
                                printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
                                printf((file_stat.st_mode & S_IXOTH) ? "x" : " ");

                                // 하드 링크의 개수 출력
                                printf("%d ", (int)file_stat.st_nlink);

                                // 소유자와 그룹 출력
                                struct passwd *owner_info = getpwuid(file_stat.st_uid);
                                struct group *group_info = getgrgid(file_stat.st_gid);
                                printf("%s %s ", owner_info->pw_name, group_info->gr_name);

                                printf("%d ", (int)file_stat.st_size);

                                // 수정 시간
                                char time_str[20];
                                strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&file_stat.st_mtime));
                                printf("%s ", time_str);

                                // 파일 이름
                                printf("%s\n", ent->d_name);
                            } else {
                                perror("파일 상태 가져오기 오류");
                            }
                        }
                    }
                    closedir(dir);
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-s") == 0) {

                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    // 파일 정보를 배열에 저장
                    struct FileInfoBySize files[1000];
                    int file_count = 0;

                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            struct stat file_stat;
                            char full_path[1024];
                            snprintf(full_path, sizeof(full_path), "%s/%s", ".", ent->d_name);

                            if (stat(full_path, &file_stat) == 0) {
                                strncpy(files[file_count].name, ent->d_name, sizeof(files[file_count].name) - 1);
                                files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
                                files[file_count].size = file_stat.st_blocks;
                                file_count++;
                            } else {
                                perror("디렉터리 열기 오류");
                            }
                        }
                    }
                    closedir(dir);

                    // 파일 크기를 기준으로 파일 정렬
                    qsort(files, file_count, sizeof(struct FileInfoBySize), compareFileInfoBySize);

                    // 정렬된 파일 출력
                    for (int j = 0; j < file_count; j++) {
                        printf("%s\t%d\n", files[j].name, (int)files[j].size);
                    }
                } else {
                    // could not open directory
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-t") == 0) {

                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    // 파일 정보를 배열에 저장
                    struct FileInfoByMTime files[1000]; 
                    int file_count = 0;

                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            struct stat file_stat;
                            char full_path[1024];
                            snprintf(full_path, sizeof(full_path), "%s/%s", ".", ent->d_name);

                            if (stat(full_path, &file_stat) == 0) {
                                strncpy(files[file_count].name, ent->d_name, sizeof(files[file_count].name) - 1);
                                files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
                                files[file_count].mtime = file_stat.st_mtime;
                                file_count++;
                            } else {
                                perror("파일 상태 가져오기 오류");
                            }
                        }
                    }
                    closedir(dir);

                    // 수정 시간을 기준으로 파일 정렬
                    qsort(files, file_count, sizeof(struct FileInfoByMTime), compareFileInfoByMTime);

                    // 정렬된 파일 출력
                    for (int j = 0; j < file_count; j++) {
                        printf("%s\t%s", files[j].name, ctime(&files[j].mtime));  // ctime converts time to a string
                    }
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-i") == 0) {

                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            struct stat file_stat;
                            char full_path[1024];
                            snprintf(full_path, sizeof(full_path), "%s/%s", ".", ent->d_name);

                            if (stat(full_path, &file_stat) == 0) {
                                printf("%ld %s\n", (long)file_stat.st_ino, ent->d_name);
                            } else {
                                perror("파일 상태 가져오기 오류");
                            }
                        }
                    }
                    closedir(dir);
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-r") == 0) {

                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    // 파일 정보를 배열에 저장
                    struct FileInfo files[1000]; 
                    int file_count = 0;

                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            strncpy(files[file_count].name, ent->d_name, sizeof(files[file_count].name) - 1);
                            files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
                            file_count++;
                        }
                    }
                    closedir(dir);

                    // 파일 이름을 기준으로 역순으로 파일 정렬
                    qsort(files, file_count, sizeof(struct FileInfo), compareFileInfoByNameReverse);

                    // 정렬된 파일 출력
                    for (int j = 0; j < file_count; j++) {
                        printf("%s\n", files[j].name);
                    }
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else if (strcmp(tokens[i], "-m") == 0) {

                DIR *dir;
                struct dirent *ent;

                if ((dir = opendir(".")) != NULL) {
                    // 쉼표로 구분된 파일 출력
                    while ((ent = readdir(dir)) != NULL) {
                        if (ent->d_name[0] != '.') { // 숨김 파일 제외
                            printf("%s, ", ent->d_name);
                        }
                    }
                    closedir(dir);

                    printf("\n");
                } else {
                    perror("디렉터리 열기 오류");
                }
            } else {
                printf("올바르지 않은 옵션: %s\n", tokens[i]);
                return;
            }
        }
    }
}

void print_current_directory() {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        printf("%s\n", buffer);
    } else {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

void change_directory(const char *path) {
    if (chdir(path) != 0) {
        perror("chdir");
    }
}

void handle_interrupt(int signo) {
    printf("\n인터럽트 발생, 프로그램을 종료합니다...\n");
    exit(EXIT_SUCCESS);
}

void copy_file(/*char *tokens[]*/const char *source, const char *destination, bool f, bool inter, bool p, bool r) {
    /*
    -f: 복사할 파일이 있을 경우 삭제하고 복사
    -i: 복사할 파일이 있을 경우 복사할 것인지 물어봄
    -p: 원본 파일의 모든 정보를 보존한 채 복사
    -r: 하위 디렉토리에 있는 모든 파일을 복사
    */

    struct stat file_stat, stat_buf;
    FILE *src, *dest;
    char buffer[1024];
    size_t bytes;
    char new_destination[PATH_MAX];

    // 원본 파일의 메타데이터 가져오기
    if (stat(source, &file_stat) != 0) {
        perror("Failed to get file statistics");
        return;
    }

    // if (stat(source, &stat_buf) != 0) {
    //     perror("Failed to get source file stats");
    //     return;
    // }

    if (stat(destination, &stat_buf) != 0) {
        if (errno != ENOENT) {
            perror("Failed to get destination stats");
            return;
        }
    }

    // 대상 경로 확인
    int dest_stat_result = stat(destination, &stat_buf);

    // 대상 경로가 디렉토리인 경우
    if (dest_stat_result == 0 && S_ISDIR(stat_buf.st_mode)) {
        const char *filename = strrchr(source, '/');

        // 원본 파일 이름만 추출 (경로 제외)
        if (filename != NULL) {
            filename++; // '/' 이후의 문자열 시작 부분
        } else {
            filename = source; // '/'가 없는 경우, 전체 경로가 파일 이름
        }

        // 새로운 대상 경로 생성
        snprintf(new_destination, PATH_MAX, "%s/%s", destination, filename);

    } else {
        strncpy(new_destination, destination, PATH_MAX);
    }

    // 파일 또는 디렉토리 존재 여부 확인
    if (access(new_destination, F_OK) == 0) {
        if (f) {
            unlink(new_destination);
        } else if (inter) {
            printf("cp: overwrite '%s'? [y/Y]", new_destination);
            char response[10];
            fgets(response, 10, stdin);

            // 사용자가 'y' 또는 'Y'로 응답하지 않으면 복사 취소
            if (response[0] != 'y' && response[0] != 'Y') {
                return;
            }
        } else {
            fprintf(stderr, "cp: '%s' already exists\n", new_destination);
            return;
        }
    }

    // '-r'
    if (r && S_ISDIR(stat_buf.st_mode)) {
        DIR *dir = opendir(source);
        if (dir == NULL) {
            perror("Failed to open source directory");
            return;
        }

        // 대상 디렉토리 생성
        mkdir(destination, stat_buf.st_mode);

        struct dirent *entry;

        // 디렉토리 내의 각 파일 및 서브디렉토리에 대해 재귀적으로 복사
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char new_source[PATH_MAX];
            // char new_destination[PATH_MAX];

            snprintf(new_source, PATH_MAX, "%s/%s", source, entry->d_name);
            snprintf(new_destination, PATH_MAX, "%s/%s", destination, entry->d_name);

            // 재귀적으로 복사 함수 호출
            copy_file(new_source, new_destination, f, inter, p, r);
        }

        closedir(dir);

    } else {
        // 파일 열기
        src = fopen(source, "rb");
        if (!src) {
            perror("Failed to open source file");
            return;
        }

        dest = fopen(new_destination, "wb");

        if (!dest) {
            perror("Failed to open destination file");
            fclose(src);
            return;
        }

        // 파일 복사
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }

        // 파일 닫기
        fclose(src);
        fclose(dest);
    }

    // '-p'
    if (p) {
        if (chmod(new_destination, file_stat.st_mode) != 0) {
            perror("Failed to copy file metadata");
            return;
        }
    }

    printf("File copied successfully\n");
}

/* 문자를 특수 형식으로 출력 */
void print_special_char(char ch, bool v, bool e, bool t) {

    // v 옵션이 활성화되어 있을 경우
    if (v) {
        // 탭 문자인 경우 t 옵션이 활성화되어 있으면 ^I로 표시
        if (ch == '\t' && t) {
            printf("^I");
        // 제어 문자인 경우
        } else if (ch < 32 || ch == 127) {
            // DEL 문자인 경우 '?'로 표시하고, 그 외는 ^와 함께 ASCII 코드로 변환하여 표시
            printf("^%c", ch == 127 ? '?' : ch + 64);
        } else {
            putchar(ch);
        }
    } else {
        putchar(ch);
    }

    // e 옵션이 활성화되어 있고, 현재 문자가 줄바꿈('\n')인 경우 줄 끝에 $ 추가
    if (ch == '\n' && e) {
        printf("$");
    }
}

/* cat - 파일 내용 처리 */
void concatenate(char *tokens[]) {
    /*
    -b: 비어있지 않은 라인에 행 번호를 붙임
    -n: 모든 라인에 행 번호를 붙임
    -v: 출력할 수 없는 문자를 출력.
    -e: -vE 옵션과 같음. 라인의 끝에 "$"를 표시함
    -t: -vT 옵션과 같음. ^I로 TAB 문자를 표시함
    */
    bool b = false, n = false, v = false, e = false, t = false;
    FILE *file;
    char ch;
    int line_number = 0;

    for (int i = 1; tokens[i] != NULL; i++) {
        // 옵션 파싱
        if (strcmp(tokens[i], "-b") == 0) {
            b = true;
        } else if (strcmp(tokens[i], "-n") == 0) {
            n = true;
        } else if (strcmp(tokens[i], "-v") == 0) {
            v = true;
        } else if (strcmp(tokens[i], "-e") == 0) {
            e = true; v = true;
        } else if (strcmp(tokens[i], "-t") == 0) {
            t = true; v = true;
        } else {
            // 파일 처리
            file = fopen(tokens[i], "r");
            if (file == NULL) {
                perror("Error opening file");
                continue;
            }

            // 새 라인 시작 플래그
            bool new_line = true;
            // 파일 끝까지 한 문자식 읽기
            while ((ch = fgetc(file)) != EOF) {
                // 새 라인 이면서 '-n' 옵션 활성화 또는 '-b' 옵션이 활성화되고 현재 줄이 공백이 아닐때
                if (new_line && (n || (b && !isspace(ch)))) {
                    // 줄 번호 출력
                    printf("%6d\t", ++line_number);
                }
                
                // v, e, t 옵션 처리
                print_special_char(ch, v, e, t);

                // 줄 바꿈 문자를 만나면 new_line = true (새 라인 시작)
                // 만나지 않으면 new_line = false
                new_line = (ch == '\n');
            }

            fclose(file);
        }
    }

    printf("\n");
}