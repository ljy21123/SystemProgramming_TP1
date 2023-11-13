#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


/* 문자를 특수 형식으로 출력 */
void print_special_char(char ch, bool v, bool e, bool t) {

    // v 옵션이 활성화되어 있을 경우
    if (v) {
        // 탭 문자인 경우 t 옵션이 활성화되어 있으면 ^I로 표시
        if (ch == '\t' && t) {
            printf("^I");
        // 제어 문자인 경우
        } else if (ch < 32 || ch == 127) {
            // DEL 문자인 경우 '?'로 표시하고, 그 외는 ^와 함께 ASCII 코드로 변환하여 표시
            printf("^%c", ch == 127 ? '?' : ch + 64);
        } else {
            putchar(ch);
        }
    } else {
        putchar(ch);
    }

    // e 옵션이 활성화되어 있고, 현재 문자가 줄바꿈('\n')인 경우 줄 끝에 $ 추가
    if (ch == '\n' && e) {
        printf("$");
    }
}

/* cat - 파일 내용 처리 */
void concatenate(char *tokens[]) {

    bool b = false, n = false, v = false, e = false, t = false;
    FILE *file;
    char ch;
    int line_number = 0;

    for (int i = 1; tokens[i] != NULL; i++) {
        if (tokens[i][0] == '-') {
            // 옵션 파싱
            for (int j = 1; tokens[i][j] != '\0'; j++) {
                switch (tokens[i][j]) {
                    case 'b': b = true; break;
                    case 'n': n = true; break;
                    case 'v': v = true; break;
                    case 'e': e = true; v = true; break;
                    case 't': t = true; v = true; break;
                    default: printf("Error: cat [-bnvet] ...");
                }
            }
        } else {
            // 파일 처리
            file = fopen(tokens[i], "r");
            if (file == NULL) {
                perror("Error opening file");
                continue;
            }

            // 새 라인 시작 플래그
            bool new_line = true;
            // 파일 끝까지 한 문자식 읽기
            while ((ch = fgetc(file)) != EOF) {
                // 새 라인 이면서 '-n' 옵션 활성화 또는 '-b' 옵션이 활성화되고 현재 줄이 공백이 아닐때
                if (new_line && (n || (b && !isspace(ch)))) {
                    // 줄 번호 출력
                    printf("%6d\t", ++line_number);
                }
                
                // v, e, t 옵션 처리
                print_special_char(ch, v, e, t);

                // 줄 바꿈 문자를 만나면 new_line = true (새 라인 시작)
                // 만나지 않으면 new_line = false
                new_line = (ch == '\n');
            }

            fclose(file);
        }
    }

    printf("\n");
}

int main(int argc, char *argv[]) {

    if (argc == 1){
        printf("사용법: cat <파일>\n");
        printf("파일의 내용을 출력합니다.\n");
        printf("\n옵션:\n");
        printf("  -b, --numbe-nonblank       비어있지 않은 라인에 행 번호를 붙임\n");
        printf("  -n, --number               모든 라인에 행 번호를 붙임\n");
        printf("  -v, --show-nonprinting     출력할 수 없는 문자를 출력\n");
        printf("  -e,                        -vE 옵션과 같음. 라인의 끝에 \"$\"를 표시함\n");
        printf("  -t,                        -vT 옵션과 같음. ^I로 TAB 문자를 표시함\n");
    }

    concatenate(argv);
    return 0;

}