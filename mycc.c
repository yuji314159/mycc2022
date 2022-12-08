#include <stdio.h>

#include "mycc.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("引数の個数が正しくありません\n");
    }

    user_input = argv[1];
    token = tokenize(user_input);
    program();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", locals ? locals->offset + 8 : 0);

    for (int i = 0; code[i]; ++i) {
        gen(code[i]);
        printf("    pop rax\n");
    }

    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");

    return 0;
}
