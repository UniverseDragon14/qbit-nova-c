#include <stdio.h>
#include <string.h>

#include "state2_vm.h"
#include "../quantum/state2.h"

static int same_name(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

void run_state2_bytecode(Instruction *code, int count) {
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
                    printf("[STATE2 WARN] only 2 qbits supported: %s\n", ins.arg1);
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
