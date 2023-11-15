#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>    // open

int main(int argc, char *argv[]) {
    int start = 0;
    if (argc < 2) {
        printf("명령어를 확인해 주세요!");
        return 0;
    }

    bool has_pipe = false;
    bool output_redirection = false;
    bool input_redirection = false;
    int pipe_index = -1;

    // 리다이렉션 심볼의 위치를 탐색
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            has_pipe = true;
            pipe_index = i;
            break; // 첫 번째 파이프를 만나면 반복문 탈출
        }
    }

    if (strcmp(argv[0], "./") == 0) // 만약 실행파일 이름이라면
        start++;

    // 파이프가 있다면
    if (has_pipe) {
        // 첫 번째 배열 동적 할당 및 값 복사
        char **firstArray = (char **)malloc((pipe_index + 1) * sizeof(char *));
        for (int i = 0, j = start; j < pipe_index; i++, j++) {
            firstArray[i] = argv[j];
        }
        firstArray[pipe_index] = NULL; // 배열 끝을 나타내기 위해 NULL 추가

        // 두 번째 배열 동적 할당 및 값 복사
        char **secondArray = (char **)malloc((argc - pipe_index) * sizeof(char *));
        for (int i = pipe_index + 1, j = 0; i < argc; i++, j++) {
            secondArray[j] = argv[i];
        }
        secondArray[argc - pipe_index - 1] = NULL;

        // 현재 파일 디스크립터 상태 저장
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stdin = dup(STDIN_FILENO);

        // 파이프 생성
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // 첫 번째 자식 프로세스 생성
        pid_t pid1 = fork();
        if (pid1 == 0) {
            // 표준 출력을 파이프의 쓰기 단으로 리디렉션
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            // 명령어 실행파일 경로를 저장할 변수
            char command_path[256];
            for (int i = 0; firstArray[i] != NULL; i++) {
                if (strcmp(firstArray[i], "<") == 0) {
                    input_redirection = true;
                } else if (strcmp(firstArray[i], ">") == 0) {
                    output_redirection = true;
                }
            }

            // 리다이렉션이 있는 경우 처리
            if (input_redirection || output_redirection) {
                strcpy(command_path, "./command/redirect_input_output");
                execvp(command_path, firstArray);
                perror("execvp first command");
                exit(EXIT_FAILURE);
            } else {
                // 입력된 명령어에 실행파일 경로 추가
                snprintf(command_path, sizeof(command_path), "./command/%s", firstArray[0]);
                // 첫 번째 명령어 실행
                execvp(command_path, firstArray);
                // execvp(firstArray[0], firstArray);
                perror("execvp first command");
                exit(EXIT_FAILURE);
            }
        } else if (pid1 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[1]); // 부모는 파이프의 쓰기 단을 사용하지 않음

        waitpid(pid1, NULL, 0);

        // 두 번째 자식 프로세스 생성
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // 표준 입력을 파이프의 읽기 단으로 리디렉션
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            // 명령어 실행파일 경로를 저장할 변수
            char command_path[256];
            for (int i = 0; secondArray[i] != NULL; i++) {
                if (strcmp(secondArray[i], "<") == 0) {
                    input_redirection = true;
                } else if (strcmp(secondArray[i], ">") == 0) {
                    output_redirection = true;
                }
            }

            // 리다이렉션이 있는 경우 처리
            if (input_redirection || output_redirection) {
                strcpy(command_path, "./command/redirect_input_output");
                execvp(command_path, secondArray);
                perror("execvp first command");
                exit(EXIT_FAILURE);
            } else {
                // 입력된 명령어에 실행파일 경로 추가
                snprintf(command_path, sizeof(command_path), "./command/%s", secondArray[0]);
                // 두 번째 명령어 실행
                execvp(command_path, secondArray);
                // execvp(secondArray[0], secondArray);
                perror("execvp second command");
                exit(EXIT_FAILURE);
            }
        } else if (pid2 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // 부모 프로세스
        close(pipe_fd[0]); // 부모는 파이프의 읽기 단을 사용하지 않음

        // 두 자식 프로세스의 종료를 기다림
        waitpid(pid2, NULL, 0);

        // 파일 디스크립터 복원
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stdin, STDIN_FILENO);

        close(saved_stdout);
        close(saved_stdin);

        free(firstArray);
        free(secondArray);
        return 0;
    }
    return 0;
}
