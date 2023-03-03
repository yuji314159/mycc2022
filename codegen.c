#include <stdio.h>

#include "mycc.h"

int labelseq = 0;

void gen_lvar(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

void load() {
    printf("    pop rax\n");    // アドレス
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
}

void store() {
    printf("    pop rdi\n");    // 値
    printf("    pop rax\n");    // アドレス
    printf("    mov [rax], rdi\n");
    printf("    push rdi\n");   // 式の値
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_RETURN:
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            return;
        case ND_IF: {
            int seq = labelseq++;
            if (node->els) {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je .Lelse%d\n", seq);
                gen(node->then);
                printf("    jmp .Lend%d\n", seq);
                printf(".Lelse%d:\n", seq);
                gen(node->els);
                printf(".Lend%d:\n", seq);
            } else {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je .Lend%d\n", seq);
                gen(node->then);
                printf(".Lend%d:\n", seq);
            }
            return;
        }
        case ND_WHILE: {
            int seq = labelseq++;
            printf(".Lbegin%d:\n", seq);
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", seq);
            gen(node->then);
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_FOR: {
            int seq = labelseq++;
            if (node->init) {
                gen(node->init);
            }
            printf(".Lbegin%d:\n", seq);
            if (node->cond) {
                gen(node->cond);
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je .Lend%d\n", seq);
            }
            gen(node->then);
            if (node->inc) {
                gen(node->inc);
            }
            printf("    jmp .Lbegin%d\n", seq);
            printf(".Lend%d:\n", seq);
            return;
        }
        case ND_BLOCK:
            for (Node *n = node->body; n; n = n->next) {
                gen(n);
                printf("    pop rax\n");
            }
            return;
        case ND_FUNCALL:
            printf("    call %s\n", node->funcname);
            printf("    push rax\n");
            return;
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_LVAR:
            gen_lvar(node);
            load();
            return;
        case ND_ASSIGN:
            gen_lvar(node->lhs);
            gen(node->rhs);
            store();
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
    }

    printf("    push rax\n");
}
