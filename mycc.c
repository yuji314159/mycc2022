#include "mycc.h"

char user_input[4096];

int main(int argc, char *argv[]) {
    if (argc != 1) {
        error("引数の個数が正しくありません\n");
    }

    fread(user_input, sizeof(user_input), 1, stdin);
    if (!feof(stdin)) {
        error("ファイルをすべて読み込めませんでした");
    }

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
