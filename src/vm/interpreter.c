#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../parser/parser.h"

#define MAX_VARS 64
#define MAX_QBITS 32

typedef struct { char name[64]; double value; } Var;
Var vars[MAX_VARS]; int var_count = 0;

typedef struct {
    char name[64];
    double a, b;
    int measured;
    int result;
} Qbit;
Qbit qbits[MAX_QBITS]; int qbit_count = 0;

double get_var(char *n) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, n) == 0) return vars[i].value;
    return 0;
}

void set_var(char *n, double v) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, n) == 0) { vars[i].value = v; return; }
    strcpy(vars[var_count].name, n);
    vars[var_count++].value = v;
}

Qbit* get_qbit(char *n) {
    for (int i = 0; i < qbit_count; i++)
        if (strcmp(qbits[i].name, n) == 0) return &qbits[i];
    return NULL;
}

void create_qbit(char *n, char *state) {
    Qbit q; memset(&q, 0, sizeof(q));
    strcpy(q.name, n);
    if (strcmp(state, "|0>") == 0) { q.a = 1.0; q.b = 0.0; }
    else if (strcmp(state, "|1>") == 0) { q.a = 0.0; q.b = 1.0; }
    else if (strcmp(state, "|+>") == 0) { q.a = 1/sqrt(2); q.b = 1/sqrt(2); }
    else if (strcmp(state, "|->") == 0) { q.a = 1/sqrt(2); q.b = -1/sqrt(2); }
    q.measured = 0;
    qbits[qbit_count++] = q;
}

void hadamard(Qbit *q) {
    double a = q->a, b = q->b;
    q->a = (a + b) / sqrt(2);
    q->b = (a - b) / sqrt(2);
    q->measured = 0;
}

void measure(Qbit *q) {
    double prob0 = q->a * q->a;
    double r = (double)rand() / RAND_MAX;
    q->result = (r > prob0) ? 1 : 0;
    q->measured = 1;
    if (q->result == 0) { q->a = 1; q->b = 0; }
    else { q->a = 0; q->b = 1; }
}

void print_qbit(Qbit *q) {
    if (q->measured) printf("|%d>\n", q->result);
    else printf("|0>=%.3f |1>=%.3f\n", q->a, q->b);
}

int eval_expr(ASTNode *node) {
    if (node->type == NODE_BINOP) {
        double left = get_var(node->left->value);
        double right = atof(node->right->value);
        if (strcmp(node->value, ">") == 0) return left > right;
        if (strcmp(node->value, "<") == 0) return left < right;
        if (strcmp(node->value, "==") == 0) return left == right;
        if (strcmp(node->value, ">=") == 0) return left >= right;
        if (strcmp(node->value, "<=") == 0) return left <= right;
    }
    return 0;
}

void execute(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_SAY: {
            Qbit *q = get_qbit(node->left->value);
            if (q) print_qbit(q);
            else if (node->left->type == NODE_STRING)
                printf("%s\n", node->left->value);
            else
                printf("%g\n", get_var(node->left->value));
            break;
        }
        case NODE_LET: {
            double val = atof(node->left->value);
            set_var(node->value, val);
            break;
        }
        case NODE_QBIT: {
            create_qbit(node->value, node->left->value);
            break;
        }
        case NODE_HADAMARD: {
            Qbit *q = get_qbit(node->value);
            if (q) hadamard(q);
            break;
        }
        case NODE_MEASURE: {
            Qbit *q = get_qbit(node->value);
            if (q) measure(q);
            break;
        }
        case NODE_CHECK: {
            if (eval_expr(node->left))
                execute(node->body);
            break;
        }
        case NODE_REPEAT: {
            int count = atoi(node->value);
            for (int i = 0; i < count; i++)
                execute(node->body);
            break;
        }
        case NODE_BLOCK: {
            execute(node->body);
            break;
        }
        default:
            break;
    }
    
    execute(node->next);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    if (argc < 2) {
        printf("QBIT NOVA AST Interpreter v0.3\n");
        printf("Usage: qnova-ast <file.qn>\n");
        return 0;
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) { printf("Error: file not found\n"); return 1; }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *code = malloc(size + 1);
    fread(code, 1, size, f);
    code[size] = 0;
    fclose(f);
    
    int count;
    Token *tokens = tokenize(code, &count);
    ASTNode *tree = parse(tokens, count);
    
    printf("=== QBIT NOVA EXECUTION ===\n");
    execute(tree->body);
    
    free_ast(tree);
    free(code);
    return 0;
}
