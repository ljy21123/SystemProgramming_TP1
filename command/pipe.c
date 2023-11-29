#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    char path[4000];
    char command_path[4500];

    strcpy(path, argv[argc - 1]);
    argv[--argc] = NULL;

    // 문자열 처리 변수
    int s_index = 0;
    int pipe_index = 0;
    int num_pipes = 0;
    char **temp_command;

    // 파이프 개수 카운트
    for(int i=0; i<argc; i++){
        if (strcmp(argv[i], "|") == 0)
            num_pipes++;
    }  

    // 리다이렉션 키워드가 있는지 저장
    bool redirection = false;

    // pipe처리를 위한 변수
    int pipes[num_pipes][2];
    pid_t pids[num_pipes + 1];
    int pids_count = num_pipes + 1;

    // 파이프들을 생성
    for (int i = 0; i < num_pipes; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    
    // 처음 파이프의 위치를 탐색
    for (int i = s_index; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }
    // 첫번째 명령어를 잘라내어 보관
    temp_command = (char **)malloc((pipe_index - s_index + 2) * sizeof(char *));
    for (int i = s_index; i < pipe_index; i++) {
            temp_command[i - s_index] = argv[i];
        }
    temp_command[pipe_index - s_index] = NULL;

    // 리다이렉션이 존재하는지 파악
    for (int i=0; temp_command[i]!= NULL;i++){
        if ((strcmp(temp_command[i], "<") == 0) || (strcmp(temp_command[i], ">") == 0)) {
            temp_command[pipe_index - s_index] = path;
            temp_command[pipe_index - s_index + 1] = NULL;
            redirection = true;
            break;
        }
    }

    // 처음은 그냥 실행
    if ((pids[0] = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pids[0] == 0) { // 첫 번째 자식 프로세스
        // 현재 자식 프로세스에서 이전 프로세스의 출력을 입력으로 받음
        close(pipes[0][0]); // 현재 프로세스의 읽기 디스크립터를 닫음
        dup2(pipes[0][1], STDOUT_FILENO); // 현재 프로세스의 출력을 파이프로 연결
        close(pipes[0][1]); // 현재 프로세스의 쓰기 디스크립터를 닫음
        if (redirection){
            snprintf(command_path, sizeof(command_path), "%sredirect_input_output", path);
            if (execvp(command_path, temp_command) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            // 입력된 명령어에 실행파일 경로 추가
            snprintf(command_path, sizeof(command_path), "%s%s", path, temp_command[0]);
            // pipe_input 프로그램 실행
            if (execvp(command_path, argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    } else if (pids[0] > 0){
        waitpid(pids[0], NULL, 0);
    }

    // 처음에 사용한 변수를 반환하고 값을 증가시킨다.
    free(temp_command);
    s_index = pipe_index + 1;
    redirection = false;

    // 2번째 이후 실행
    for(int i=1; i<pids_count;i++){
        if ((pids[i] = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pids[i] == 0) { // 자식 프로세스
            // 처리할 파이프의 위치를 찾는다.
            pipe_index = argc;
            for (int j = s_index; j < argc; j++) {
                if (strcmp(argv[j], "|") == 0) {
                    pipe_index = j;
                    break;
                }
            }
            // 명령어를 임시 저장할 배열
            temp_command = (char **)malloc((pipe_index - s_index + 1) * sizeof(char *));
            // 명령어에서 필요한 일부분만 복사하여 저장
            for (int j = s_index; j < pipe_index; j++) {
                temp_command[j - s_index] = argv[j];
            }
            temp_command[pipe_index - s_index] = NULL;

            // 리다이렉션이 존재하는지 파악
            for (int k=0; temp_command[k]!= NULL;k++){
                if ((strcmp(temp_command[k], "<") == 0) || (strcmp(temp_command[k], ">") == 0)) {
                    temp_command[pipe_index - s_index] = path;
                    temp_command[pipe_index - s_index + 1] = NULL;
                    redirection = true;
                    break;
                }
            }

            // 시작 위치를 파이프 다음으로 넘긴다
            s_index = pipe_index + 1;

            // 현재 자식 프로세스에서 이전 프로세스의 출력을 입력으로 받음
            close(pipes[i-1][1]); // 현재 프로세스의 쓰기 디스크립터를 닫음
            dup2(pipes[i-1][0], STDIN_FILENO); // 이전 프로세스의 출력을 현재 프로세스의 입력으로 복제
            close(pipes[i-1][0]); // 이전 프로세스의 읽기 디스크립터를 닫음
            
            if (i != (pids_count - 1)){
                // 현재 자식 프로세스의 출력을 다음 프로세스의 입력으로 연결
                close(pipes[i][0]); // 현재 프로세스의 읽기 디스크립터를 닫음
                dup2(pipes[i][1], STDOUT_FILENO); // 현재 프로세스의 출력을 파이프로 연결
                close(pipes[i][1]); // 현재 프로세스의 쓰기 디스크립터를 닫음
            }
            if (redirection){
                snprintf(command_path, sizeof(command_path), "%sredirect_input_output", path);
                if (execvp(command_path, temp_command) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } else {
                // 입력된 명령어에 실행파일 경로 추가
                snprintf(command_path, sizeof(command_path), "%s%s", path, temp_command[0]);
                // 명령어 수행
                if (execvp(command_path, temp_command) == -1) {
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }
            }
            free(temp_command);    
            exit(EXIT_SUCCESS);
        } else if (pids[i] > 0) {
            for (int j = s_index; j < argc; j++) {
                if (strcmp(argv[j], "|") == 0) {
                    pipe_index = j;
                    break;
                }
            }
            s_index = pipe_index + 1;
            waitpid(pids[i], NULL, 0);
        }
    }

    return 0;
}
