#ifndef PARSER_H
#define PARSER_H
#include "../lexer/lexer.h"

typedef enum {
    NODE_PROGRAM,
    NODE_SAY,
    NODE_LET,
    NODE_CHECK,
    NODE_BLOCK,
    NODE_QBIT,
    NODE_HADAMARD,
    NODE_MEASURE,
    NODE_ENTANGLE,
    NODE_REPEAT,
    NODE_BINOP,
    NODE_STRING,
    NODE_NUMBER,
    NODE_IDENT,
    NODE_STATE
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char value[256];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *body;
    struct ASTNode *next;
} ASTNode;

ASTNode* parse(Token *tokens, int count);
void print_ast(ASTNode *node, int depth);
void free_ast(ASTNode *node);

#endif
