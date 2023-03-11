#include "mycc.h"

Node *code[100];
LVar *locals;

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_node_unary(NodeKind kind, Node *expr) {
    Node *node = new_node(kind);
    node->lhs = expr;
    return node;
}

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Node *new_node_lvar(LVar *lvar) {
    Node *node = new_node(ND_LVAR);
    node->lvar = lvar;
    return node;
}

LVar *push_lvar(char *name) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = name;
    locals = lvar;
    return lvar;
}

LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (strlen(var->name) == tok->len && memcmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
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
            node->funcname = strndup(tok->str, tok->len);
            node->args = funcargs();
            return node;
        }

        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            lvar = push_lvar(strndup(tok->str, tok->len));
        }
        return new_node_lvar(lvar);
    }

    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) {
        return unary();
    } else if (consume("-")) {
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    } else {
        return primary();
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

    LVar *head = push_lvar(expect_ident());
    while (consume(",")) {
        push_lvar(expect_ident());
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

Function *program() {
    Function head;
    head.next = NULL;
    Function *cur = &head;

    while (!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }
    return head.next;
}
