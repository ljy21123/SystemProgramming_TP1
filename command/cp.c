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

void copy_file(/*char *tokens[]*/const char *source, const char *destination, bool f, bool inter, bool p, bool r) {

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

int main(int argc, char *argv[]) {

    if (argc == 1){
        printf("사용법: cp <파일/폴더> <파일/폴더>\n");
        printf("파일이나 폴더를 복사합니다.\n");
        printf("\n옵션:\n");
        printf("  -f:  복사할 파일이 있을 경우 삭제하고 복사\n");
        printf("  -i:  복사할 파일이 있을 경우 복사할 것인지 물어봄\n");
        printf("  -p:  원본 파일의 모든 정보를 보존한 채 복사\n");
        printf("  -r:  하위 디렉토리에 있는 모든 파일을 복사\n");
    }

    bool f = false, inter = false, p = false, r = false;
    char *source = NULL, *destination = NULL;

    // 옵션 파싱
    for (int i = 1; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            f = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            inter = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            p = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            r = true;
        } else {
            if (source == NULL) {
                source = argv[i];
            } else {
                destination = argv[i];
            }
        }
    }

    if (source != NULL && destination != NULL) {
        copy_file(source, destination, f, inter, p, r);
    } else {
        fprintf(stderr, "Usage: cp [-fipr] source destination\n");
    }

    return 0;

}