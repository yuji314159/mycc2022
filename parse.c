#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mycc.h"

char *user_input;
Token *token;
Node *code[100];
LVar *locals;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

bool consume_return() {
    if (token->kind != TK_RETURN) {
        return false;
    }
    token = token->next;
    return true;
}

bool consume_if() {
    if (token->kind != TK_IF) {
        return false;
    }
    token = token->next;
    return true;
}

bool consume_else() {
    if (token->kind != TK_ELSE) {
        return false;
    }
    token = token->next;
    return true;
}

bool consume_while() {
    if (token->kind != TK_WHILE) {
        return false;
    }
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return false;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

bool expect(char *op) {
    if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            memcmp(token->str, op, token->len)) {
        error_at(token->str, "\"%s\" ではありません", op);
    }
    token = token->next;
}

int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            ++p;
        } else if (
                memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0 ||
                memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
        } else if (
                *p == '+' || *p == '-' || *p == '*' || *p == '/' ||
                *p == '(' || *p == ')' || *p == '<' || *p == '>' ||
                *p == '=' || *p == ';') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
        } else if (strncmp(p, "return", 6) == 0 && !isalnum(p[6]) && p[6] != '_') {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
        } else if (strncmp(p, "if", 2) == 0 && !isalnum(p[2]) && p[2] != '_') {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
        } else if (strncmp(p, "else", 4) == 0 && !isalnum(p[4]) && p[4] != '_') {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
        } else if (strncmp(p, "while", 5) == 0 && !isalnum(p[5]) && p[5] != '_') {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
        } else if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalnum(p[len]) || p[len] == '_') {
                ++len;
            }
            cur = new_token(TK_IDENT, cur, p, len);
            p += len;
        } else if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
        } else {
            error_at(p, "トークナイズできません");
        }
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len = tok->len && memcmp(var->name, tok->str, var->len) == 0) {
            return var;
        }
    }
    return NULL;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_return(Node *lhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = lhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_lvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(tok);
    if (lvar) {
        node->offset = lvar->offset;
    } else {
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = tok->str;
        lvar->len = tok->len;
        lvar->offset = locals ? locals->offset + 8 : 0;
        node->offset = lvar->offset;
        locals = lvar;
    }
    return node;
}

Node *expr();

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        return new_node_lvar(tok);
    }

    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else {
        return primary();
    }
}

Node *mul() {
    Node *node = unary();

    for(;;) {
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node *add() {
    Node *node = mul();

    for(;;) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<")) {
            node = new_node(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_node(ND_LE, node, add());
        } else if (consume(">")) {
            // lhs > rhs == rhs < lhs
            node = new_node(ND_LT, add(), node);
        } else if (consume(">=")) {
            // lhs >= rhs == rhs <= lhs
            node = new_node(ND_LE, add(), node);
        } else {
            return node;
        }
    }
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node *assign() {
    Node *node = equality();

    for(;;) {
        if (consume("=")) {
            node = new_node(ND_ASSIGN, node, assign());
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
    if (consume_return()) {
        node = new_node_return(expr());
        expect(";");
    } else if (consume_if()) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume_else()) {
            node->els = stmt();
        }
    } else if (consume_while()) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
    } else {
        node = expr();
        expect(";");
    }
    return node;
}

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}
