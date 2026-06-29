#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../parser/parser.h"

typedef enum {
    OP_SAY, OP_LET, OP_QBIT, OP_HADAMARD, OP_MEASURE,
    OP_CHECK, OP_REPEAT, OP_BLOCK, OP_END
} OpCode;

typedef struct {
    OpCode op;
    char arg1[64];
    char arg2[64];
    int value;
} Instruction;

#define MAX_INSTR 1024
Instruction bytecode[MAX_INSTR];
int instr_count = 0;

void emit(OpCode op, char *arg1, char *arg2, int val) {
    bytecode[instr_count].op = op;
    if(arg1) strcpy(bytecode[instr_count].arg1, arg1);
    if(arg2) strcpy(bytecode[instr_count].arg2, arg2);
    bytecode[instr_count].value = val;
    instr_count++;
}

void compile(ASTNode *node) {
    switch(node->type) {
        case NODE_SAY: emit(OP_SAY, node->left->value, NULL, 0); break;
        case NODE_LET: emit(OP_LET, node->value, node->left->value, 0); break;
        case NODE_QBIT: emit(OP_QBIT, node->value, node->left->value, 0); break;
        case NODE_HADAMARD: emit(OP_HADAMARD, node->value, NULL, 0); break;
        case NODE_MEASURE: emit(OP_MEASURE, node->value, NULL, 0); break;
        case NODE_CHECK: compile(node->left); emit(OP_CHECK, NULL, NULL, 0); compile(node->body); emit(OP_END, NULL, NULL, 0); break;
        case NODE_REPEAT: emit(OP_REPEAT, NULL, NULL, atoi(node->value)); compile(node->body); emit(OP_END, NULL, NULL, 0); break;
        case NODE_BLOCK: compile(node->body); break;
    }
    compile(node->next);
}

void save_qbc(const char *filename) {
    FILE *f = fopen(filename, "wb");
    fwrite(&instr_count, sizeof(int), 1, f);
    fwrite(bytecode, sizeof(Instruction), instr_count, f);
    fclose(f);
    printf("QBC saved: %s (%d instructions)\n", filename, instr_count);
}

int load_qbc(const char *filename) {
    FILE *f = fopen(filename, "rb");
    fread(&instr_count, sizeof(int), 1, f);
    fread(bytecode, sizeof(Instruction), instr_count, f);
    fclose(f);
    printf("QBC loaded: %s (%d instructions)\n", filename, instr_count);
    return 1;
}

void print_bytecode() {
    const char *names[] = {"SAY","LET","QBIT","HADAMARD","MEASURE","CHECK","REPEAT","BLOCK","END"};
    for(int i=0; i<instr_count; i++) {
        printf("%04d: %s", i, names[bytecode[i].op]);
        if(bytecode[i].arg1[0]) printf(" %s", bytecode[i].arg1);
        if(bytecode[i].arg2[0]) printf(" %s", bytecode[i].arg2);
        if(bytecode[i].value) printf(" %d", bytecode[i].value);
        printf("\n");
    }
}
