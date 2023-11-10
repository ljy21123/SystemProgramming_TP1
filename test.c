/*
    파일 재지향 테스트를 위한 코드입니다...
    예) ./test < a.txt
    예2) ./test > b.txt
*/

#include <stdio.h>

int main() {
    char buffer[256];
    // 표준 입력에서 데이터를 읽음
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        printf("%s\n", buffer);
    } else {
        printf("input x\n");
    }

    return 0;
}