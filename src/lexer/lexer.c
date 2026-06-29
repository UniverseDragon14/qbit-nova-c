#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

#define MAX_TOKENS 1024

static Token tokens[MAX_TOKENS];
static int tok_count = 0;

static void add_token(TokenType type, const char *val, int line) {
    tokens[tok_count].type = type;
    strncpy(tokens[tok_count].value, val, 255);
    tokens[tok_count].line = line;
    tok_count++;
}

const char* token_name(TokenType t) {
    switch(t) {
        case TOKEN_SAY:       return "SAY";
        case TOKEN_LET:       return "LET";
        case TOKEN_CHECK:     return "CHECK";
        case TOKEN_OTHERWISE: return "OTHERWISE";
        case TOKEN_REPEAT:    return "REPEAT";
        case TOKEN_ACTION:    return "ACTION";
        case TOKEN_QBIT:      return "QBIT";
        case TOKEN_HADAMARD:  return "HADAMARD";
        case TOKEN_MEASURE:   return "MEASURE";
        case TOKEN_ENTANGLE:  return "ENTANGLE";
        case TOKEN_STATE_0:   return "STATE|0>";
        case TOKEN_STATE_1:   return "STATE|1>";
        case TOKEN_STATE_PLUS:return "STATE|+>";
        case TOKEN_STATE_MINUS:return "STATE|->";
        case TOKEN_EQ:        return "=";
        case TOKEN_EQEQ:      return "==";
        case TOKEN_GT:        return ">";
        case TOKEN_LT:        return "<";
        case TOKEN_GTE:       return ">=";
        case TOKEN_LTE:       return "<=";
        case TOKEN_IDENT:     return "IDENT";
        case TOKEN_NUMBER:    return "NUMBER";
        case TOKEN_STRING:    return "STRING";
        case TOKEN_LBRACE:    return "{";
        case TOKEN_RBRACE:    return "}";
        case TOKEN_DIRECTIVE: return "DIRECTIVE";
        case TOKEN_EOF:       return "EOF";
        default:              return "UNKNOWN";
    }
}

Token* tokenize(const char *src, int *count) {
    tok_count = 0;
    int i = 0, line = 1;
    int len = strlen(src);

    while (i < len) {
        /* Skip whitespace */
        if (src[i] == '\n') { line++; i++; continue; }
        if (isspace(src[i])) { i++; continue; }

        /* Directive @ */
        if (src[i] == '@') {
            char buf[256]; int j=0;
            i++;
            while (i<len && !isspace(src[i]) && src[i]!='\n')
                buf[j++]=src[i++];
            buf[j]=0;
            add_token(TOKEN_DIRECTIVE, buf, line);
            continue;
        }

        /* String */
        if (src[i] == '"') {
            char buf[256]; int j=0;
            i++;
            while (i<len && src[i]!='"') buf[j++]=src[i++];
            buf[j]=0; i++;
            add_token(TOKEN_STRING, buf, line);
            continue;
        }

        /* Quantum state |0> |1> |+> |-> */
        if (src[i]=='|') {
            char n = src[i+1];
            if (n=='0') { add_token(TOKEN_STATE_0,"|0>",line); i+=3; continue; }
            if (n=='1') { add_token(TOKEN_STATE_1,"|1>",line); i+=3; continue; }
            if (n=='+') { add_token(TOKEN_STATE_PLUS,"|+>",line); i+=3; continue; }
            if (n=='-') { add_token(TOKEN_STATE_MINUS,"|->",line); i+=3; continue; }
        }

        /* Operators */
        if (src[i]=='=' && src[i+1]=='=') { add_token(TOKEN_EQEQ,"==",line); i+=2; continue; }
        if (src[i]=='>' && src[i+1]=='=') { add_token(TOKEN_GTE,">=",line); i+=2; continue; }
        if (src[i]=='<' && src[i+1]=='=') { add_token(TOKEN_LTE,"<=",line); i+=2; continue; }
        if (src[i]=='=') { add_token(TOKEN_EQ,"=",line); i++; continue; }
        if (src[i]=='>') { add_token(TOKEN_GT,">",line); i++; continue; }
        if (src[i]=='<') { add_token(TOKEN_LT,"<",line); i++; continue; }
        if (src[i]=='{') { add_token(TOKEN_LBRACE,"{",line); i++; continue; }
        if (src[i]=='}') { add_token(TOKEN_RBRACE,"}",line); i++; continue; }

        /* Numbers */
        if (isdigit(src[i])) {
            char buf[64]; int j=0;
            while (i<len && isdigit(src[i])) buf[j++]=src[i++];
            buf[j]=0;
            add_token(TOKEN_NUMBER, buf, line);
            continue;
        }

        /* Keywords + Identifiers */
        if (isalpha(src[i]) || src[i]=='_') {
            char buf[256]; int j=0;
            while (i<len && (isalnum(src[i])||src[i]=='_'))
                buf[j++]=src[i++];
            buf[j]=0;

            if (!strcmp(buf,"say"))       add_token(TOKEN_SAY,buf,line);
            else if (!strcmp(buf,"let"))  add_token(TOKEN_LET,buf,line);
            else if (!strcmp(buf,"check"))add_token(TOKEN_CHECK,buf,line);
            else if (!strcmp(buf,"otherwise")) add_token(TOKEN_OTHERWISE,buf,line);
            else if (!strcmp(buf,"repeat"))add_token(TOKEN_REPEAT,buf,line);
            else if (!strcmp(buf,"action"))add_token(TOKEN_ACTION,buf,line);
            else if (!strcmp(buf,"qbit")) add_token(TOKEN_QBIT,buf,line);
            else if (!strcmp(buf,"hadamard")) add_token(TOKEN_HADAMARD,buf,line);
            else if (!strcmp(buf,"measure"))  add_token(TOKEN_MEASURE,buf,line);
            else if (!strcmp(buf,"entangle")) add_token(TOKEN_ENTANGLE,buf,line);
            else add_token(TOKEN_IDENT, buf, line);
            continue;
        }
        i++;
    }
    add_token(TOKEN_EOF, "EOF", line);
    *count = tok_count;
    return tokens;
}
