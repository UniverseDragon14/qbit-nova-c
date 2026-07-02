#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../compiler/compiler.h"
#include "../vm/bytecode.h"

#define MAX_QASM_QBITS 32

static char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: file not found: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *code = malloc(size + 1);
    if (!code) {
        fclose(f);
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    fread(code, 1, size, f);
    code[size] = 0;
    fclose(f);

    return code;
}

static int qbit_index(char qnames[MAX_QASM_QBITS][64], int qcount, const char *name) {
    for (int i = 0; i < qcount; i++) {
        if (strcmp(qnames[i], name) == 0) return i;
    }
    return -1;
}

static void export_openqasm(void) {
    char qnames[MAX_QASM_QBITS][64];
    int qcount = 0;

    for (int i = 0; i < instr_count; i++) {
        Instruction ins = bytecode[i];

        if (ins.op == OP_QBIT) {
            if (qcount >= MAX_QASM_QBITS) {
                fprintf(stderr, "Error: QASM exporter supports max %d qbits\n", MAX_QASM_QBITS);
                return;
            }

            if (qbit_index(qnames, qcount, ins.arg1) == -1) {
                strncpy(qnames[qcount], ins.arg1, 63);
                qnames[qcount][63] = 0;
                qcount++;
            }
        }
    }

    if (qcount == 0) {
        fprintf(stderr, "Error: no qbits found for QASM export\n");
        return;
    }

    printf("OPENQASM 3.0;\n");
    printf("include \"stdgates.inc\";\n\n");

    printf("qubit[%d] q;\n", qcount);
    printf("bit[%d] c;\n\n", qcount);

    printf("// QBIT NOVA name map\n");
    for (int i = 0; i < qcount; i++) {
        printf("// %s -> q[%d]\n", qnames[i], i);
    }
    printf("\n");

    for (int i = 0; i < instr_count; i++) {
        Instruction ins = bytecode[i];

        if (ins.op == OP_HADAMARD) {
            int q = qbit_index(qnames, qcount, ins.arg1);
            if (q >= 0) printf("h q[%d];\n", q);
            else printf("// WARN: unknown qbit for h: %s\n", ins.arg1);
        }

        else if (ins.op == OP_CNOT) {
            int control = qbit_index(qnames, qcount, ins.arg1);
            int target = qbit_index(qnames, qcount, ins.arg2);

            if (control >= 0 && target >= 0) {
                printf("cx q[%d], q[%d];\n", control, target);
            } else {
                printf("// WARN: unknown qbit for cx: %s %s\n", ins.arg1, ins.arg2);
            }
        }

        else if (ins.op == OP_MEASURE) {
            int q = qbit_index(qnames, qcount, ins.arg1);
            if (q >= 0) printf("c[%d] = measure q[%d];\n", q, q);
            else printf("// WARN: unknown qbit for measure: %s\n", ins.arg1);
        }

        else if (ins.op == OP_ENTANGLE) {
            printf("// NOTE: entangle %s %s is simulator-level; use h + cx for hardware QASM.\n",
                   ins.arg1, ins.arg2);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: qnova-qasm <file.qn>\n");
        return 0;
    }

    char *source = read_file(argv[1]);
    if (!source) return 1;

    int token_count = 0;
    Token *tokens = tokenize(source, &token_count);
    ASTNode *tree = parse(tokens, token_count);

    instr_count = 0;
    compile(tree->body);

    export_openqasm();

    free_ast(tree);
    free(source);

    return 0;
}
