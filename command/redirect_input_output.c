#include <unistd.h>   // fork, execvp, dup2, close
#include <sys/types.h>
#include <sys/wait.h>  // wait
#include <fcntl.h>    // open
#include <stdio.h>    // perror, fprintf, stderr
#include <stdlib.h>   // exit
#include <string.h> 

// 절대경로 저장 변수
char path[4000];

// 리다이렉션 처리 함수
void handleRedirection(char* tokens[]) {
    char command_path[4500];
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);

    int input_redirection = -1;
    int output_redirection = -1;

    // 리다이렉션 심볼의 위치를 탐색
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) 
            input_redirection = i;
        else if (strcmp(tokens[i], ">") == 0) 
            output_redirection = i;
    }

    // 입력 리다이렉션 처리
    if (input_redirection != -1) {
        // 입력 리다이렉션 처리
        // 파일을 읽기 전용으로 열고 파일 디스크립터 저장
        int fd = open(tokens[input_redirection + 1], O_RDONLY);
        if (fd < 0) {
            perror("open");
            return;
        }
        // fd를 표준입력으로 설정
        dup2(fd, 0);
        close(fd);
        // 토큰에서 리다이렉션 토큰과 파일을 제거
        tokens[input_redirection] = NULL;
        tokens[input_redirection + 1] = NULL;
    }

    // 출력 리다이렉션 처리
    if (output_redirection != -1) {
        // 출력 리다이렉션 처리
        // 파일을 여는데 쓰기 전용이며, 없으면 생성하고, 이미 파일이 있을때 존재하는 내용을 지운다
        int fd = open(tokens[output_redirection + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("open");
            return;
        }
        dup2(fd, 1);
        close(fd);
        // 토큰에서 리다이렉션 토큰과 파일을 제거
        tokens[output_redirection] = NULL;
        tokens[output_redirection + 1] = NULL;
    }

    // 자식 프로세스 생성
    pid_t pid = fork();
    
    if (pid == 0){
        // 입력된 명령어에 실행파일 경로 추가
        snprintf(command_path, sizeof(command_path), "%s%s", path, tokens[0]);
        // 경로에 있는 실행파일 실행
        execvp(command_path, tokens);

        // 파일 리다이렉션 실험을 위한 코드
        // execvp(tokens[1], tokens);
        
        // 실행파일이 없다면 오류출력
        fprintf(stderr, "%s: Command not found\n", tokens[0]);
    } else if (pid > 0){
        wait((int *) 0);
    } else{
        perror("fork failed");
    }

    // 파일 디스크립터를 복원
    dup2(saved_stdout, 1);
    dup2(saved_stdin, 0);
    close(saved_stdout);
    close(saved_stdin);
}

int main(int argc, char *argv[]){
    if (argc == 2){
        return 0;
    }
    strcpy(path, argv[argc - 1]);
    argv[--argc] = NULL;
    handleRedirection(argv);
    return 0;
}