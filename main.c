
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
        
        // exit입력시 쉘 종료
        if (strcmp(input, "exit") == 0) 
            break;

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