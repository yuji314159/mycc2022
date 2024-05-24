#include "mycc.h"

int labelseq = 0;
char *argregs1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argregs8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen(Node *node);

void gen_lvar(Node *node) {
    switch (node->kind) {
        case ND_LVAR:
            if (node->lvar->is_local) {
                printf("    mov rax, rbp\n");
                printf("    sub rax, %d\n", node->lvar->offset);
                printf("    push rax\n");
            } else {
                printf("    lea rax, %s[rip]\n", node->lvar->name);
                printf("    push rax\n");
            }
            return;
        case ND_DEREF:
            gen(node->lhs);
            return;
    }

    error("代入の左辺値が変数ではありません");
}

void load(Type *type) {
    printf("    pop rax\n");    // アドレス
    if (size_of(type) == 1) {
        printf("    movsx rax, BYTE PTR [rax]\n");
    } else {
        printf("    mov rax, [rax]\n");
    }
    printf("    push rax\n");
}

void store(Type *type) {
    printf("    pop rdi\n");    // 値
    printf("    pop rax\n");    // アドレス
    if (size_of(type) == 1) {
        printf("    mov [rax], dil\n");
    } else {
        printf("    mov [rax], rdi\n");
    }
    printf("    push rdi\n");   // 式の値
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NULL:
            return;
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
        case ND_FUNCALL: {
            int nargs = 0;
            for (Node *arg = node->args; arg; arg = arg->next) {
                gen(arg);
                ++nargs;
            }

            for (int i = nargs - 1; i >= 0; --i) {
                printf("    pop %s\n", argregs8[i]);
            }

            // RSP must be aligned to a 16 byte boundary
            int seq = labelseq++;
            printf("    mov rax, rsp\n");
            printf("    and rax, 15\n");    // RAX = RAX % 16 = RAX & 15
            printf("    jnz .Lcall%d\n", seq);
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    jmp .Lend%d\n", seq);
            printf(".Lcall%d:\n", seq);
            printf("    sub rsp, 8\n");
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    add rsp, 8\n");
            printf(".Lend%d:\n", seq);
            printf("    push rax\n");
            return;
        }
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_LVAR:
            printf("# %s\n", node->lvar->name);
            gen_lvar(node);
            // 配列はポインターのように扱うため、アドレスのままにする (loadしない)
            if (node->type->type != TY_ARRAY) {
                load(node->type);
            }
            return;
        case ND_ASSIGN:
            if (node->type->type == TY_ARRAY) {
                error("代入の左辺値に配列は指定できません");
            }
            gen_lvar(node->lhs);
            gen(node->rhs);
            store(node->type);
            return;
        case ND_ADDR:
            gen_lvar(node->lhs);
            return;
        case ND_DEREF:
            gen(node->lhs);
            // 配列はポインターのように扱うため、アドレスのままにする (loadしない)
            if (node->type->type != TY_ARRAY) {
                load(node->type);
            }
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            if (node->type->type == TY_PTR || node->type->type == TY_ARRAY) {
                printf("    imul rdi, 8\n");
            }
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            if (node->type->type == TY_PTR || node->type->type == TY_ARRAY) {
                printf("    imul rdi, 8\n");
            }
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

void codegen(Program *prog) {
    printf(".intel_syntax noprefix\n");

    // data section
    printf(".data\n");
    for (LVar *lvar = prog->globals; lvar; lvar = lvar->next) {
        printf("%s:\n", lvar->name);
        // TODO: 1次元配列のみ対応
        size_t len = 8;
        if (lvar->type->type == TY_ARRAY) {
            len *= lvar->type->len;
        }
        printf("    .zero %d\n", len);
    }

    // text section
    printf(".text\n");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);

        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        int i = fn->param_count;
        for (LVar *lvar = fn->params; lvar; lvar = lvar->next) {
            printf("# %s\n", lvar->name);
            if (size_of(lvar->type) == 1) {
                printf("    mov [rbp - %d], %s\n", lvar->offset, argregs1[--i]);
            } else {
                printf("    mov [rbp - %d], %s\n", lvar->offset, argregs8[--i]);
            }
        }

        for (Node *node = fn->node; node; node = node->next) {
            gen(node);
        }
    }
}
