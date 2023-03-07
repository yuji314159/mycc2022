#include "mycc.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("引数の個数が正しくありません\n");
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Function *prog = program();

    for (Function *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (LVar *lvar = fn->locals; lvar; lvar = lvar->next) {
            lvar->offset = offset;
            offset += 8;
        }
        fn->stack_size = offset;
    }

    printf(".intel_syntax noprefix\n");

    for (Function *fn = prog; fn; fn = fn->next) {
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);

        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        for (Node *node = fn->node; node; node = node->next) {
            gen(node);
            printf("    pop rax\n");
        }

        // printf("    mov rsp, rbp\n");
        // printf("    pop rbp\n");
        // printf("    ret\n");
    }

    return 0;
}
