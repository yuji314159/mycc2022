#include "mycc.h"

Node *code[100];
LVar *locals;
LVar *globals;

void debug_type(Type *type) {
    printf("#");
    if (type->type == TY_INT) {
        printf(" int");
    } else if (type->type == TY_PTR) {
        printf(" pointer to");
        debug_type(type->ptr_to);
    } else if (type->type == TY_ARRAY) {
        printf(" array[%d] of", type->len);
        debug_type(type->ptr_to);
    } else if (type->type == TY_INT) {
        printf(" int");
    } else {
        printf(" ERROR");
    }
    printf("\n");
}

Type *int_type() {
    Type *type = calloc(1, sizeof(Type));
    type->type = TY_INT;
    return type;
}

Type *ptr_to(Type *base) {
    Type *type = calloc(1, sizeof(Type));
    type->type = TY_PTR;
    type->ptr_to = base;
    return type;
}

Type *array_of(Type *base, size_t len) {
    Type *type = calloc(1, sizeof(Type));
    type->type = TY_ARRAY;
    type->ptr_to = base;
    type->len = len;
    return type;
}

Type *char_type() {
    Type *type = calloc(1, sizeof(Type));
    type->type = TY_CHAR;
    return type;
}

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_node_unary(NodeKind kind, Node *expr) {
    Node *node = new_node(kind);
    if (kind == ND_ADDR) {
        if (expr->type->type == TY_ARRAY) {
            node->type = ptr_to(expr->type->ptr_to);
        } else {
            node->type = ptr_to(expr->type);
        }
    } else if (kind == ND_DEREF) {
        if (expr->type->type == TY_PTR || expr->type->type == TY_ARRAY) {
            node->type = expr->type->ptr_to;
        } else {
            error("この値はderefできません");
        }
    } else {
        node->type = expr->type;
    }
    node->lhs = expr;
    return node;
}

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    if (kind == ND_ASSIGN) {
        node->type = lhs->type;
    } else if (kind == ND_ADD) {
        // rhsの型がポインターだったら左右を入れ替える
        if (rhs->type->type == TY_PTR || rhs->type->type == TY_ARRAY) {
            Node *tmp = lhs;
            lhs = rhs;
            rhs = tmp;
        }
        // それでもrhsの型がポインターだったらエラー (ポインター同士の足し算はできない)
        if (rhs->type->type == TY_PTR || rhs->type->type == TY_ARRAY) {
            error("ポインター同士の足し算はできません");
        }
        node->type = lhs->type;
    } else if (kind == ND_SUB) {
        // rhsの型がポインターだったらエラー (ポインターの引き算はできない)
        if (rhs->type->type == TY_PTR || rhs->type->type == TY_ARRAY) {
            error("ポインターの引き算はできません");
        }
        node->type = lhs->type;
    } else {
        // 本来は `==` とかの型はboolになる?
        node->type = int_type();
    }
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->type = int_type();
    node->val = val;
    return node;
}

Node *new_node_lvar(LVar *lvar) {
    Node *node = new_node(ND_LVAR);
    node->type = lvar->type;
    node->lvar = lvar;
    return node;
}

LVar *push_lvar(char *name, Type *type, bool is_local) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->name = name;
    lvar->type = type;
    lvar->is_local = is_local;

    if (is_local) {
        lvar->next = locals;
        locals = lvar;
    } else {
        lvar->next = globals;
        globals = lvar;
    }

    return lvar;
}

LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (strlen(var->name) == tok->len && memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }

    for (LVar *var = globals; var; var = var->next) {
        if (strlen(var->name) == tok->len && memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }

    return NULL;
}

char *new_label() {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".L.data.%d", cnt++);
    return strndup(buf, 20);
}

Node *expr();

Node *funcargs() {
    if (consume(")")) {
        return NULL;
    }

    Node *head = expr();
    Node *cur = head;
    while (consume(",")) {
        cur->next = expr();
        cur = cur->next;
    }
    expect(")");
    return head;
}

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
            Node *node = new_node(ND_FUNCALL);
            // TODO: 関数の戻り値の型を `int` と仮定している
            node->type = int_type();
            node->funcname = strndup(tok->str, tok->len);
            node->args = funcargs();
            return node;
        }

        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            error("未定義の変数です");
        }
        return new_node_lvar(lvar);
    }

    tok = consume_str();
    if (tok) {
        Type *type = array_of(char_type(), tok->contents_len);
        LVar *lvar = push_lvar(new_label(), type, false);
        lvar->initial_contents = tok->contents;
        lvar->initial_contents_len = tok->contents_len;
        return new_node_lvar(lvar);
    }

    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) {
        return unary();
    } else if (consume("-")) {
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    } else if (consume("&")) {
        return new_node_unary(ND_ADDR, unary());
    } else if (consume("*")) {
        return new_node_unary(ND_DEREF, unary());
    } else if (consume("sizeof")) {
        Node *node = unary();
        return new_node_num(size_of(node->type));
    } else {
        Node *node = primary();
        if (consume("[")) {
            // `x[y]` は `*(x + y)` と等価
            node = new_node_unary(ND_DEREF, new_node_binary(ND_ADD, node, expr()));
            expect("]");
        }
        return node;
    }
}

Node *mul() {
    Node *node = unary();

    for(;;) {
        if (consume("*")) {
            node = new_node_binary(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node_binary(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node *add() {
    Node *node = mul();

    for(;;) {
        if (consume("+")) {
            node = new_node_binary(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node_binary(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<")) {
            node = new_node_binary(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_node_binary(ND_LE, node, add());
        } else if (consume(">")) {
            // lhs > rhs == rhs < lhs
            node = new_node_binary(ND_LT, add(), node);
        } else if (consume(">=")) {
            // lhs >= rhs == rhs <= lhs
            node = new_node_binary(ND_LE, add(), node);
        } else {
            return node;
        }
    }
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_node_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node_binary(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node *assign() {
    Node *node = equality();

    for(;;) {
        if (consume("=")) {
            node = new_node_binary(ND_ASSIGN, node, assign());
        } else {
            return node;
        }
    }
}

Node *expr() {
    return assign();
}

Node *stmt() {
    Node *node;
    if (consume("return")) {
        node = new_node_unary(ND_RETURN, expr());
        expect(";");
    } else if (consume("if")) {
        node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
    } else if (consume("while")) {
        node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
    } else if (consume("for")) {
        node = new_node(ND_FOR);
        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = expr();
            expect(")");
        }
        node->then = stmt();
    } else if (consume("{")) {
        Node head;
        head.next = NULL;
        Node *cur = &head;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        node = new_node(ND_BLOCK);
        node->body = head.next;
    } else if (consume("int")) {
        node = new_node(ND_NULL);
        Type *type = int_type();
        while (consume("*")) {
            type = ptr_to(type);
        }
        char *name = expect_ident();
        // TODO: 1次元配列のみ対応
        if (consume("[")) {
            type = array_of(type, expect_number());
            expect("]");
        }
        LVar *head = push_lvar(name, type, true);
        expect(";");
    } else if (consume("char")) {
        node = new_node(ND_NULL);
        Type *type = char_type();
        while (consume("*")) {
            type = ptr_to(type);
        }
        char *name = expect_ident();
        // TODO: 1次元配列のみ対応
        if (consume("[")) {
            type = array_of(type, expect_number());
            expect("]");
        }
        LVar *head = push_lvar(name, type, true);
        expect(";");
    } else {
        node = expr();
        expect(";");
    }
    return node;
}

LVar *funcparams() {
    if (consume(")")) {
        return NULL;
    }

    Type *type;
    if (consume("char")) {
        type = char_type();
    } else {
        expect("int");
        type = int_type();
    }
    while (consume("*")) {
        type = ptr_to(type);
    }
    char *name = expect_ident();
    // TODO: 1次元配列のみ対応
    if (consume("[")) {
        type = array_of(type, expect_number());
        expect("]");
    }
    push_lvar(name, type, true);
    while (consume(",")) {
        Type *type;
        if (consume("char")) {
            type = char_type();
        } else {
            expect("int");
            type = int_type();
        }
        while (consume("*")) {
            type = ptr_to(type);
        }
        char *name = expect_ident();
        // TODO: 1次元配列のみ対応
        if (consume("[")) {
            type = array_of(type, expect_number());
            expect("]");
        }
        push_lvar(name, type, true);
    }
    expect(")");
    return locals;
}

int param_count(LVar *lvar) {
    int i = 0;
    while (lvar) lvar = lvar->next, ++i;
    return i;
}

Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    // TODO: 関数の戻り値の型は int だけに制限
    expect("int");
    fn->name = expect_ident();
    expect("(");
    fn->params = funcparams();
    fn->param_count = param_count(fn->params);
    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals;
    return fn;
}

void global_var() {
    Type *type;
    if (consume("char")) {
        type = char_type();
    } else {
        expect("int");
        type = int_type();
    }
    while (consume("*")) {
        type = ptr_to(type);
    }
    char *name = expect_ident();
    // TODO: 1次元配列のみ対応
    if (consume("[")) {
        type = array_of(type, expect_number());
        expect("]");
    }
    expect(";");
    push_lvar(name, type, false);
}

bool is_function() {
    Token *tok = token;

    if (consume("char")) {
    } else {
        expect("int");
    }
    while (consume("*")) {}
    expect_ident();
    // TODO: 1次元配列のみ対応
    if (consume("[")) {
        expect_number();
        expect("]");
    }
    bool isfunc = consume("(");

    token = tok;
    return isfunc;
}

Program *program() {
    Function head;
    head.next = NULL;
    Function *cur = &head;
    globals = NULL;

    while (!at_eof()) {
        if (is_function()) {
            cur->next = function();
            cur = cur->next;
        } else {
            global_var();
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->fns = head.next;
    prog->globals = globals;
    return prog;
}
