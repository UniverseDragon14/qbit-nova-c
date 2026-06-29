#ifndef QBIT_NOVA_COMPILER_H
#define QBIT_NOVA_COMPILER_H

#include "../parser/parser.h"
#include "../vm/bytecode.h"

extern Instruction bytecode[MAX_INSTR];
extern int instr_count;

void emit(OpCode op, char *arg1, char *arg2, int val);
void compile(ASTNode *node);
void save_qbc(const char *filename);
int load_qbc(const char *filename);
void print_bytecode(void);

#endif
