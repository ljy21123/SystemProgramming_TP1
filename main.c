
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
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

int get_tokens(char *cmd, char **tokens);

int main(int argc, char *argv[]){
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];
    int narg;
    pid_t pid;

    while (1) {
        // 프롬프트 표시
        printf("MyShell> ");

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
            break;
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
            if (pipe || inputRedirect || outputRedirect){
                // 경로에 있는 실행파일 실행
                execvp("./command/pipe", tokens);
            }
            else {
                // 명령어 실행파일 경로를 저장할 변수
                char command_path[256];
                // 입력된 명령어에 실행파일 경로 추가
                snprintf(command_path, sizeof(command_path), "./command/%s", tokens[0]);
                // 경로에 있는 실행파일 실행
                execvp(command_path, tokens);
                // execvp(tokens[0], tokens);
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
