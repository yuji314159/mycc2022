typedef enum {
    TK_RESERVED,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
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
    ND_LVAR,
    ND_RETURN,
    ND_IF,
    ND_WHILE,
    ND_FOR,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;

    // if/while/for statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    int val;
    int offset;
};

typedef struct LVar LVar;

struct LVar {
    LVar *next;
    char *name;
    int len;
    int offset;
};

extern char *user_input;
extern Token *token;
extern Node *code[100];
extern LVar *locals;
void error(char *fmt, ...);
Token *tokenize(char *p);
void program();

void gen(Node *node);
