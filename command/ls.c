#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h> 

struct FileInfo {
    char name[256];
};

int compareFileInfoByName(const void *a, const void *b) {
    return strcmp(((struct FileInfo *)a)->name, ((struct FileInfo *)b)->name);
}
int compareFileReverse(const void *a, const void *b) {
    return strcmp(((struct FileInfo *)b)->name, ((struct FileInfo *)a)->name);
}


void list_directory(const char *path, int show_hidden, int show_detail, int show_inode, int show_size, int sort_time, int show_commas, int show_reverse) {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(path);

    struct FileInfo files[1000]; 
    int file_count = 0;

    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        strncpy(files[file_count].name, entry->d_name, sizeof(files[file_count].name) - 1);
        files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
        file_count++;
    }
    closedir(dir);

    // 파일 이름을 기준으로 파일 정렬
    qsort(files, file_count, sizeof(struct FileInfo), compareFileInfoByName);
    

    if (show_reverse) {
        qsort(files, file_count, sizeof(struct FileInfo), compareFileReverse);
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
        }
    }

    if (sort_time) {
        struct stat *file_stats = malloc(file_count * sizeof(struct stat));
        if (file_stats == NULL) {
            perror("Memory allocation error");
            return;
        }
        if (!show_reverse){
            qsort(files, file_count, sizeof(struct FileInfo), compareFileReverse);
        } else {
            qsort(files, file_count, sizeof(struct FileInfo), compareFileInfoByName);
        }

        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);

            if (stat(full_path, &file_stats[i]) == -1) {
                perror("Error getting file stats");
                continue;
            }
        }

        if (!show_reverse){
            // 파일의 수정 시간을 기준으로 정렬
            for (int i = 0; i < file_count - 1; i++) {
                for (int j = i + 1; j < file_count; j++) {
                    if (file_stats[j].st_mtime > file_stats[i].st_mtime) {
                        // 파일의 수정 시간을 비교하여 위치 교환
                        struct FileInfo temp = files[i];
                        files[i] = files[j];
                        files[j] = temp;

                        struct stat temp_stat = file_stats[i];
                        file_stats[i] = file_stats[j];
                        file_stats[j] = temp_stat;
                    }
                }
            }
        } else {
            // 파일의 수정 시간을 기준으로 반대로 정렬
            for (int i = 0; i < file_count - 1; i++) {
                for (int j = i + 1; j < file_count; j++) {
                    if (file_stats[j].st_mtime < file_stats[i].st_mtime) { 
                        // 파일의 수정 시간을 비교하여 위치 교환
                        struct FileInfo temp = files[i];
                        files[i] = files[j];
                        files[j] = temp;

                        struct stat temp_stat = file_stats[i];
                        file_stats[i] = file_stats[j];
                        file_stats[j] = temp_stat;
                    }
                }
            }
        }


        free(file_stats);
    }

    // 파일의 상세 정보 출력
    if (show_detail) {
        // 파일 타입 및 허가권 출력
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
            
            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) {
                perror("Error getting file stats");
                continue;
            }

            // 파일 상세 정보 출력
            printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
            printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
            printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
            printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
            printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
            printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
            printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
            printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
            printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

            // 하드 링크 수, 소유자, 그룹, 파일 크기, 수정 시간 출력
            printf(" %ld", file_stat.st_nlink);
            struct passwd *pw = getpwuid(file_stat.st_uid);
            struct group *gr = getgrgid(file_stat.st_gid);
            printf(" %s %s", pw->pw_name, gr->gr_name);
            printf(" %ld", file_stat.st_size);

            // 수정 시간
            char time_str[20];
            strftime(time_str, sizeof(time_str), " %b %d %H:%M", localtime(&file_stat.st_mtime));
            printf("%s ", time_str);

            
            printf("%s\n", files[i].name);
        }
        
        // 파일 이름
    } else if(show_inode) {
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);

            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) {
                perror("Error getting file stats");
                continue;
            }
            printf("%lu %s\n", file_stat.st_ino, files[i].name);
        }

    } else if (show_size) {
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);

            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) {
                perror("Error getting file stats");
                continue;
            }

            printf("%s %ld\n", files[i].name, file_stat.st_blocks / 2);
        }
    } else if(show_commas) {
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
            
            printf("%s", files[i].name);
            if(i != file_count - 1)
                printf(", ");
            else 
                printf("\n");
        }

    } else {
        for (int i = 0; i < file_count; i++) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, files[i].name);
            printf("%s\n", files[i].name);
        }
    }
}

void parse_options_and_list_directory(int argc, char *argv[]) {
    const char *path = "."; // 기본 경로는 현재 디렉토리
    int show_hidden = 0;
    int show_detail = 0;
    int show_inode = 0;
    int show_size = 0;
    int sort_time = 0;
    int show_commas = 0;
    int show_reverse = 0;

    // 옵션 파싱
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a': show_hidden = 1; break;   //완료
                    case 'l': show_detail = 1; break;   //완료
                    case 'i': show_inode = 1; break;    //완료
                    case 's': show_size = 1; break;     //완료
                    case 't': sort_time = 1; break;     //완료
                    case 'm': show_commas = 1; break;   //완료
                    case 'r': show_reverse = 1; break;  //완료?
                    default:
                        fprintf(stderr, "Usage: ls [-alismr] [directory]\n");
                        return;
                }
            }
        } else {
            path = argv[i];
        }
    }

    // 파싱된 옵션을 기반으로 디렉토리 목록 출력
    list_directory(path, show_hidden, show_detail, show_inode, show_size, sort_time, show_commas, show_reverse);
}

int main(int argc, char *argv[]) {
    parse_options_and_list_directory(argc, argv);
    return 0;
}
