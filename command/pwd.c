#include <stdio.h>
#include <dirent.h>
#include <string.h>

void print_current_directory() {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        printf("%s\n", buffer);
    } else {
        perror("getcwd");
    }
}

int main(int argc, char *argv[]) {
    print_current_directory();
    return 0;
}