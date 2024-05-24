#include "mycc.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("引数の個数が正しくありません\n");
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Program *prog = program();

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        int offset = 0;
        for (LVar *lvar = fn->locals; lvar; lvar = lvar->next) {
            offset += size_of(lvar->type);
            lvar->offset = offset;
        }
        // 8 byte単位でalignする
        fn->stack_size = (offset + 8 - 1) / 8 * 8;
    }

    codegen(prog);

    return 0;
}
