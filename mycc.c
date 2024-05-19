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
            if (lvar->type->type == TY_ARRAY) {
                offset += 8 * lvar->type->len;
            } else {
                offset += 8;
            }
            lvar->offset = offset;
        }
        fn->stack_size = offset;
    }

    codegen(prog);

    return 0;
}
