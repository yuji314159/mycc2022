#include "mycc.h"

char *user_input;
Token *token;

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

char *strndup(const char *s, size_t n) {
    char *t = malloc(n + 1);
    strncpy(t, s, n);
    t[n] = '\0';
    return t;
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

Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

Token *consume_str() {
    if (token->kind != TK_STR) {
        return NULL;
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

char *expect_ident() {
    if (token->kind != TK_IDENT) {
        error_at(token->str, "identifierではありません");
    }
    char *s = strndup(token->str, token->len);
    token = token->next;
    return s;
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

bool starts_with(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

char *starts_with_reserved(char *p) {
    static char *kws[] = {"return", "if", "else", "while", "for", "int", "sizeof", "char"};
    for (int i = 0; i < sizeof(kws) / sizeof(*kws); ++i) {
        char next = p[strlen(kws[i])];
        if (starts_with(p, kws[i]) && !isalnum(next) && next != '_') {
            return kws[i];
        }
    }

    static char *ops[] = {"<=", ">=", "==", "!="};
    for (int i = 0; i < sizeof(ops) / sizeof(*ops); ++i) {
        if (starts_with(p, ops[i])) {
            return ops[i];
        }
    }

    if (strchr("+-*/()<>=;{},&[]", p[0])) {
        return strndup(p, 1);
    }

    return NULL;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            ++p;
            continue;
        }

        char *kw = starts_with_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalnum(p[len]) || p[len] == '_') {
                ++len;
            }
            cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        if (*p == '"') {
            char *q = p++;
            while (*p != '\0' && *p != '"') {
                p++;
            }
            if (*p == '\0') {
                error_at(q, "文字列リテラルが閉じられていません");
            }
            p++;    // skip '"'

            cur = new_token(TK_STR, cur, q, p - q);
            cur->contents = strndup(q + 1, p - q - 2);
            cur->contents_len = p - q - 1;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
