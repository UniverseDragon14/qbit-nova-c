#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "quantum/state2.h"

static char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error: file not found: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *code = malloc(size + 1);
    if (!code) {
        fclose(f);
        printf("Error: memory allocation failed\n");
        return NULL;
    }

    fread(code, 1, size, f);
    code[size] = 0;
    fclose(f);

    return code;
}

static int same_name(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static void run_state2_bytecode(Instruction *code, int count) {
    QState2 state;
    qstate2_init_00(&state);

    char q0[64] = "";
    char q1[64] = "";
    int qcount = 0;

    int measured = 0;
    int bit0 = 0;
    int bit1 = 0;

    printf("\n=== QBIT NOVA STATE2 VM ===\n");

    for (int pc = 0; pc < count; pc++) {
        Instruction ins = code[pc];

        switch (ins.op) {
            case OP_SAY:
                if (same_name(ins.arg1, q0)) {
                    if (measured) printf("|%d>\n", bit0);
                    else qstate2_print(&state);
                } else if (same_name(ins.arg1, q1)) {
                    if (measured) printf("|%d>\n", bit1);
                    else qstate2_print(&state);
                } else {
                    printf("%s\n", ins.arg1);
                }
                break;

            case OP_QBIT:
                if (qcount == 0) {
                    strcpy(q0, ins.arg1);
                    qcount++;
                    printf("[STATE2] qbit %s mapped as first qbit\n", q0);
                } else if (qcount == 1) {
                    strcpy(q1, ins.arg1);
                    qcount++;
                    printf("[STATE2] qbit %s mapped as second qbit\n", q1);
                } else {
                    printf("[STATE2 WARN] only 2 qbits supported in this runner: %s\n", ins.arg1);
                }
                break;

            case OP_HADAMARD:
                if (same_name(ins.arg1, q0)) {
                    qstate2_h_first(&state);
                    printf("[STATE2] H(%s) ", ins.arg1);
                    qstate2_print(&state);
                } else {
                    printf("[STATE2 TODO] H currently supports first qbit only: %s\n", ins.arg1);
                }
                break;

            case OP_CNOT:
                if (same_name(ins.arg1, q0) && same_name(ins.arg2, q1)) {
                    qstate2_cnot_first_to_second(&state);
                    printf("[STATE2] CNOT %s,%s ", ins.arg1, ins.arg2);
                    qstate2_print(&state);
                } else {
                    printf("[STATE2 TODO] CNOT currently supports first -> second only: %s %s\n",
                           ins.arg1, ins.arg2);
                }
                break;

            case OP_MEASURE: {
                int result = qstate2_measure(&state);
                measured = 1;
                bit0 = (result >> 1) & 1;
                bit1 = result & 1;

                printf("[STATE2] MEASURE pair ");
                qstate2_print(&state);
                break;
            }

            default:
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    if (argc < 2) {
        printf("Usage: qnova-state2 <file.qn>\n");
        return 0;
    }

    char *source = read_file(argv[1]);
    if (!source) return 1;

    int token_count = 0;
    Token *tokens = tokenize(source, &token_count);
    ASTNode *tree = parse(tokens, token_count);

    compile(tree->body);

    printf("=== QBIT NOVA BYTECODE ===\n");
    print_bytecode();

    run_state2_bytecode(bytecode, instr_count);

    free_ast(tree);
    free(source);

    return 0;
}
