#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_LT,
    ND_LE,
    // ND_GT,
    // ND_GE,
    ND_EQ,
    ND_NE,
    ND_ASSIGN,
    ND_ADDR,
    ND_DEREF,
    ND_LVAR,
    ND_RETURN,
    ND_IF,
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,
    ND_FUNCALL,
    ND_NULL,
} NodeKind;

typedef struct Type Type;

struct Type {
    enum {
        TY_INT,
        TY_PTR,
        TY_ARRAY,
    } type;
    struct Type *ptr_to;
    size_t len;
};

typedef struct LVar LVar;

struct LVar {
    LVar *next;
    char *name;
    int offset;
    Type *type;
};

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *next;
    Type *type;

    // operator
    Node *lhs;
    Node *rhs;

    // if/while/for statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // block
    Node *body;

    // function call
    char *funcname;
    Node *args;

    int val;
    LVar *lvar;
};

typedef struct Function Function;

struct Function {
    Function *next;
    char *name;
    LVar *params;
    int param_count;

    Node *node;
    LVar *locals;
    int stack_size;
};

extern char *user_input;
extern Token *token;
void error(char *fmt, ...);
char *strndup(const char *s, size_t n);
bool consume(char *op);
Token *consume_ident();
bool expect(char *op);
char *expect_ident();
int expect_number();
bool at_eof();
Token *tokenize(char *p);

extern Node *code[100];
extern LVar *locals;
Function *program();

void codegen(Function *prog);

void debug_type(Type *type);
