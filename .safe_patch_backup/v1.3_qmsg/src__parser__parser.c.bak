#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static Token *toks;
static int pos;
static int tok_total;

static Token* peek() { return &toks[pos]; }
static Token* advance() { return &toks[pos++]; }

static int check_type(TokenType t) {
    return toks[pos].type == t;
}

static ASTNode* new_node(NodeType type, const char *val) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = type;
    if (val) strncpy(n->value, val, 255);
    return n;
}

static ASTNode* parse_block();
static ASTNode* parse_stmt();

static ASTNode* parse_expr() {
    /* identifier OP number */
    ASTNode *left = new_node(NODE_IDENT, peek()->value);
    advance();
    ASTNode *op = new_node(NODE_BINOP, peek()->value);
    advance();
    ASTNode *right = new_node(NODE_NUMBER, peek()->value);
    advance();
    op->left = left;
    op->right = right;
    return op;
}

static ASTNode* parse_block() {
    advance(); /* skip { */
    ASTNode *block = new_node(NODE_BLOCK, "block");
    ASTNode *last = NULL;
    while (!check_type(TOKEN_RBRACE) &&
           !check_type(TOKEN_EOF)) {
        ASTNode *s = parse_stmt();
        if (!block->body) block->body = s;
        else last->next = s;
        last = s;
    }
    advance(); /* skip } */
    return block;
}

static ASTNode* parse_stmt() {
    Token *t = peek();

    /* say */
    if (t->type == TOKEN_SAY) {
        advance();
        ASTNode *n = new_node(NODE_SAY, "say");
        Token *val = advance();
        if (val->type == TOKEN_STRING)
            n->left = new_node(NODE_STRING, val->value);
        else
            n->left = new_node(NODE_IDENT, val->value);
        return n;
    }

    /* let x = 10 */
    if (t->type == TOKEN_LET) {
        advance();
        ASTNode *n = new_node(NODE_LET, peek()->value);
        advance(); /* name */
        advance(); /* = */
        n->left = new_node(NODE_NUMBER, peek()->value);
        advance();
        return n;
    }

    /* check expr { block } */
    if (t->type == TOKEN_CHECK) {
        advance();
        ASTNode *n = new_node(NODE_CHECK, "check");
        n->left = parse_expr();
        n->body = parse_block();
        return n;
    }

    /* repeat 3 { block } */
    if (t->type == TOKEN_REPEAT) {
        advance();
        ASTNode *n = new_node(NODE_REPEAT, peek()->value);
        advance();
        n->body = parse_block();
        return n;
    }

    /* qbit q = |0> */
    if (t->type == TOKEN_QBIT) {
        advance();
        ASTNode *n = new_node(NODE_QBIT, peek()->value);
        advance(); /* name */
        advance(); /* = */
        n->left = new_node(NODE_STATE, peek()->value);
        advance();
        return n;
    }

    /* hadamard q */
    if (t->type == TOKEN_HADAMARD) {
        advance();
        ASTNode *n = new_node(NODE_HADAMARD, peek()->value);
        advance();
        return n;
    }

    /* measure q */
    if (t->type == TOKEN_MEASURE) {
        advance();
        ASTNode *n = new_node(NODE_MEASURE, peek()->value);
        advance();
        return n;
    }

    /* entangle q p */
    if (t->type == TOKEN_ENTANGLE) {
        advance();
        ASTNode *n = new_node(NODE_ENTANGLE, peek()->value);
        advance();
        n->left = new_node(NODE_IDENT, peek()->value);
        advance();
        return n;
    }

    /* cnot q p */
    if (t->type == TOKEN_CNOT) {
        advance();
        ASTNode *n = new_node(NODE_CNOT, peek()->value);
        advance();
        n->left = new_node(NODE_IDENT, peek()->value);
        advance();
        return n;
    }
    if (t->type == TOKEN_ACTION) {
        advance();
        ASTNode *n = new_node(NODE_ACTION, peek()->value);
        advance();
        return n;
    }

    /* skip directive + unknown */
    advance();
    return new_node(NODE_IDENT, t->value);
}

ASTNode* parse(Token *tokens, int count) {
    toks = tokens;
    pos = 0;
    tok_total = count;
    ASTNode *prog = new_node(NODE_PROGRAM, "program");
    ASTNode *last = NULL;
    while (!check_type(TOKEN_EOF)) {
        if (check_type(TOKEN_DIRECTIVE)) { advance(); continue; }
        ASTNode *s = parse_stmt();
        if (!prog->body) prog->body = s;
        else last->next = s;
        last = s;
    }
    return prog;
}

void print_ast(ASTNode *node, int depth) {
    if (!node) return;
    for (int i=0;i<depth;i++) printf("  ");
    printf("[%d] '%s'\n", node->type, node->value);
    print_ast(node->left, depth+1);
    print_ast(node->right, depth+1);
    print_ast(node->body, depth+1);
    print_ast(node->next, depth);
}

void free_ast(ASTNode *n) {
    if (!n) return;
    free_ast(n->left);
    free_ast(n->right);
    free_ast(n->body);
    free_ast(n->next);
    free(n);
}
