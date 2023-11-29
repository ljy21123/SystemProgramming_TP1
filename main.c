
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
    2023-11-13: 쉘 구조 변경 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

void change_directory(const char *path);
int get_tokens(char *cmd, char **tokens);
void handle_interrupt(int signo);

int main(int argc, char *argv[]){
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];
    int narg;
    pid_t pid;
    char path[4000]; // 실행 프로그램이 있는 절대 경로 저장
    // 명령어 실행파일 경로를 저장할 변수
    char command_path[4500];
    if (signal(SIGINT, handle_interrupt) == SIG_ERR || signal(SIGTSTP, handle_interrupt) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }
    
    if(getcwd(path, sizeof(path))==NULL){
        perror("getcwd");
        exit(1);
    }
    strcat(path, "/command/"); // 실행프로그램 절대 경로 생성

    while (1) {
        char pwd[4000];
        if(getcwd(pwd, sizeof(pwd))==NULL){
            perror("getcwd");
            exit(1);
        }
        // 프롬프트 표시
        printf("MyShell: %s> ", pwd);

        // 사용자 입력 받기
        fgets(input, MAX_INPUT_SIZE, stdin);

        // 엔터 키만 입력한 경우 루프 다시 시작
        if (strlen(input) == 0 || input[0] == '\n')
            continue;

        // 개행 문자 제거하고 명령어의 끝을 표기
        input[strcspn(input, "\n")] = '\0';

        // 입력 토큰화
        narg = get_tokens(input, tokens);

        // exit입력시 쉘 종료
        if (strcmp(input, "exit") == 0){
            printf("bye\n");
            exit(0);
        }
        // 디렉터리 변경 
        if (strcmp(tokens[0], "cd") == 0) {
            if (narg > 1) {
                change_directory(tokens[1]);
            } else {
                // Handle "cd" without arguments (change to home directory)
                change_directory(getenv("HOME"));
            }
            continue; // Skip the fork-exec logic for "cd" command
        }

        // 분할 토큰 출력
        // printf("%d\n", narg);
        // for(int i=0; tokens[i]!=NULL; i++)
        //     printf("%s ", tokens[i]);

        bool background = false;
        // 백그라운드 확인
        for (int i = 0; tokens[i] != NULL; i++) {
            // '&' 문자가 토큰의 끝에 있는지 확인
            if (tokens[i][strlen(tokens[i]) - 1] == '&') {
                background = true;
                // '&' 문자 제거
                tokens[i][strlen(tokens[i]) - 1] = '\0';
                // 빈 토큰인 경우 넘어감
                if (strlen(tokens[i]) == 0) {
                    tokens[i] = NULL;
                }
                break;
            }
        }

        // 리다이렉션 및 파이프 확인
        bool pipe = false;
        bool inputRedirect = false;
        bool outputRedirect = false;
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "<") == 0)
                inputRedirect = true;
            if (strcmp(tokens[i], ">") == 0)
                outputRedirect = true;
            if (strcmp(tokens[i], "|") == 0) 
                pipe = true;
        }

        // if (pipe || inputRedirect || outputRedirect)
        //     tokens[0]=handleRedirection(tokens);

        // 자식 프로세스 생성
        pid = fork();
        
        if (pid == 0){
            if (pipe){
                // 토크 마지막에 절대경로 추가
                tokens[narg++] = path;
                tokens[narg] = NULL;
                snprintf(command_path, sizeof(command_path), "%spipe", path);
                // 경로에 있는 실행파일 실행
                execvp(command_path, tokens);
            }
            else if(inputRedirect || outputRedirect){
                // 토크 마지막에 절대경로 추가
                tokens[narg++] = path;
                tokens[narg] = NULL;
                snprintf(command_path, sizeof(command_path), "%sredirect_input_output", path);
                execvp(command_path, tokens);
            }
            else {
                // command에 들어있는 프로그램이 아닌 다른 주소의 프로그램 실행시
                if (strstr(tokens[0], "/") != NULL) {
                    // tokens[0]에 "/"이 포함되어 있다.
                    execvp(tokens[0], tokens);
                } else {
                    // 입력된 명령어에 실행파일 경로 추가
                    snprintf(command_path, sizeof(command_path), "%s%s", path, tokens[0]);
                    // 경로에 있는 실행파일 실행
                    execvp(command_path, tokens);
                    // execvp(tokens[0], tokens);
                }
            }
            // 실행파일이 없다면 오류출력
            fprintf(stderr, "%s: Command not found\n", tokens[0]);
        } else if (pid > 0){
            if (!background) {
                // 백그라운드가 아닌 경우에만 기다림
                 wait((int *) 0);
            } else {
                printf("Process running in background with PID: %d\n", pid);
            }
        } else {
            perror("fork failed");
        }
    }
    return 0;
}

// 입력 명령어 토큰화 함수
int get_tokens(char *cmd, char **tokens){
    int i = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    return i;
}

void handle_interrupt(int signo) {
    printf("\n인터럽트 발생, 프로그램을 종료합니다...\n");
    exit(EXIT_SUCCESS);
}

void change_directory(const char *path) {
    if (chdir(path) != 0) {
        perror("chdir");
    }
}

//경로 문제 확인을 위한 main에 구현한 pwd(추후삭제)
void print_current_directory() {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        printf("%s\n", buffer);
    } else {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}