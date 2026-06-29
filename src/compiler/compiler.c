#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../parser/parser.h"
#include "../vm/bytecode.h"

Instruction bytecode[MAX_INSTR];
int instr_count = 0;

void emit(OpCode op, char *arg1, char *arg2, int val) {
    bytecode[instr_count].op = op;

    bytecode[instr_count].arg1[0] = '\0';
    bytecode[instr_count].arg2[0] = '\0';

    if (arg1) strcpy(bytecode[instr_count].arg1, arg1);
    if (arg2) strcpy(bytecode[instr_count].arg2, arg2);

    bytecode[instr_count].value = val;
    instr_count++;
}

void compile(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_SAY:
            emit(OP_SAY, node->left->value, NULL, 0);
            break;

        case NODE_LET:
            emit(OP_LET, node->value, node->left->value, 0);
            break;

        case NODE_QBIT:
            emit(OP_QBIT, node->value, node->left->value, 0);
            break;

        case NODE_HADAMARD:
            emit(OP_HADAMARD, node->value, NULL, 0);
            break;

        case NODE_MEASURE:
            emit(OP_MEASURE, node->value, NULL, 0);
            break;

        case NODE_ENTANGLE:
            emit(OP_ENTANGLE, node->value, node->left ? node->left->value : NULL, 0);
            break;

        case NODE_CNOT:
            emit(OP_CNOT, node->value, node->left ? node->left->value : NULL, 0);
            break;

        case NODE_CHECK:
            if (node->left && node->left->left && node->left->right) {
                emit(OP_CHECK, node->left->left->value, node->left->value, atoi(node->left->right->value));
            } else {
                emit(OP_CHECK, NULL, NULL, 0);
            }
            compile(node->body);
            emit(OP_END, NULL, NULL, 0);
            break;

        case NODE_REPEAT:
            emit(OP_REPEAT, NULL, NULL, atoi(node->value));
            compile(node->body);
            emit(OP_END, NULL, NULL, 0);
            break;

        case NODE_ACTION:
            emit(OP_SAFE_ACTION, node->value, NULL, 0);
            break;

        case NODE_BLOCK:
            compile(node->body);
            break;

        default:
            break;
    }

    compile(node->next);
}

void save_qbc(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Error: cannot write QBC file: %s\n", filename);
        return;
    }

    fwrite(&instr_count, sizeof(int), 1, f);
    fwrite(bytecode, sizeof(Instruction), instr_count, f);
    fclose(f);

    printf("QBC saved: %s (%d instructions)\n", filename, instr_count);
}

int load_qbc(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Error: cannot read QBC file: %s\n", filename);
        return 0;
    }

    fread(&instr_count, sizeof(int), 1, f);
    fread(bytecode, sizeof(Instruction), instr_count, f);
    fclose(f);

    printf("QBC loaded: %s (%d instructions)\n", filename, instr_count);
    return 1;
}

void print_bytecode() {
    const char *names[] = {
        "SAY", "LET", "QBIT", "HADAMARD", "MEASURE",
        "ENTANGLE", "CNOT", "CHECK", "REPEAT", "BLOCK",
        "END", "SAFE_ACTION"
    };

    for (int i = 0; i < instr_count; i++) {
        int op = bytecode[i].op;
        const char *name = "UNKNOWN";

        if (op >= 0 && op < (int)(sizeof(names) / sizeof(names[0]))) {
            name = names[op];
        }

        printf("%04d: %s", i, name);

        if (bytecode[i].arg1[0]) printf(" %s", bytecode[i].arg1);
        if (bytecode[i].arg2[0]) printf(" %s", bytecode[i].arg2);
        if (bytecode[i].value) printf(" %d", bytecode[i].value);

        printf("\n");
    }
}
