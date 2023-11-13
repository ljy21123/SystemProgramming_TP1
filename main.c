#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100

int get_tokens(char *cmd, char **tokens);

int main(){
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

        // 분할 토큰 출력
        // printf("%d\n", narg);
        // for(int i=0; tokens[i]!=NULL; i++)
        //     printf("%s ", tokens[i]);

        pid = fork();
    
        if (pid == 0)
            execvp(tokens[0], tokens);
        else if (pid > 0)
            wait((int *) 0);
        else
            perror("fork failed");
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