#ifndef LEXER_H
#define LEXER_H

typedef enum {
    /* Keywords */
    TOKEN_SAY, TOKEN_LET, TOKEN_CHECK,
    TOKEN_OTHERWISE, TOKEN_REPEAT,
    TOKEN_ACTION, TOKEN_QBIT,
    TOKEN_HADAMARD, TOKEN_MEASURE,
    TOKEN_ENTANGLE,

    /* Quantum states */
    TOKEN_STATE_0, TOKEN_STATE_1,
    TOKEN_STATE_PLUS, TOKEN_STATE_MINUS,

    /* Operators */
    TOKEN_EQ, TOKEN_GT, TOKEN_LT,
    TOKEN_GTE, TOKEN_LTE, TOKEN_EQEQ,

    /* Primitives */
    TOKEN_IDENT, TOKEN_NUMBER, TOKEN_STRING,

    /* Symbols */
    TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_DIRECTIVE, TOKEN_EOF, TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
} Token;

Token* tokenize(const char *source, int *count);
const char* token_name(TokenType t);

#endif
