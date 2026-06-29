#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "bytecode.h"

#define MAX_VM_VARS 64
#define MAX_VM_QBITS 32

typedef struct {
    char name[64];
    double value;
} VMVar;

typedef struct {
    char name[64];
    double a;
    double b;
    int measured;
    int result;
} VMQbit;

static VMVar vars[MAX_VM_VARS];
static int var_count = 0;

static VMQbit qbits[MAX_VM_QBITS];
static int qbit_count = 0;

static VMVar* find_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return &vars[i];
    }
    return NULL;
}

static void set_var(const char *name, double value) {
    VMVar *v = find_var(name);
    if (v) {
        v->value = value;
        return;
    }

    if (var_count >= MAX_VM_VARS) {
        printf("[VM ERROR] variable limit reached\n");
        return;
    }

    strcpy(vars[var_count].name, name);
    vars[var_count].value = value;
    var_count++;
}

static VMQbit* find_qbit(const char *name) {
    for (int i = 0; i < qbit_count; i++) {
        if (strcmp(qbits[i].name, name) == 0) return &qbits[i];
    }
    return NULL;
}

static void create_qbit(const char *name, const char *state) {
    VMQbit *existing = find_qbit(name);
    VMQbit *q = existing;

    if (!q) {
        if (qbit_count >= MAX_VM_QBITS) {
            printf("[VM ERROR] qbit limit reached\n");
            return;
        }
        q = &qbits[qbit_count++];
        memset(q, 0, sizeof(VMQbit));
        strcpy(q->name, name);
    }

    if (strcmp(state, "|0>") == 0) {
        q->a = 1.0;
        q->b = 0.0;
    } else if (strcmp(state, "|1>") == 0) {
        q->a = 0.0;
        q->b = 1.0;
    } else if (strcmp(state, "|+>") == 0) {
        q->a = 1.0 / sqrt(2.0);
        q->b = 1.0 / sqrt(2.0);
    } else if (strcmp(state, "|->") == 0) {
        q->a = 1.0 / sqrt(2.0);
        q->b = -1.0 / sqrt(2.0);
    } else {
        printf("[VM ERROR] unknown qbit state: %s\n", state);
        q->a = 1.0;
        q->b = 0.0;
    }

    q->measured = 0;
    q->result = -1;
}

static void print_qbit(VMQbit *q) {
    if (q->measured) {
        printf("|%d>\n", q->result);
    } else {
        printf("|0>=%.3f |1>=%.3f\n", q->a, q->b);
    }
}

static void hadamard(VMQbit *q) {
    double a = q->a;
    double b = q->b;

    q->a = (a + b) / sqrt(2.0);
    q->b = (a - b) / sqrt(2.0);
    q->measured = 0;
    q->result = -1;
}

static void measure(VMQbit *q) {
    double prob0 = q->a * q->a;
    double r = (double)rand() / RAND_MAX;

    q->result = (r > prob0) ? 1 : 0;
    q->measured = 1;

    if (q->result == 0) {
        q->a = 1.0;
        q->b = 0.0;
    } else {
        q->a = 0.0;
        q->b = 1.0;
    }
}

static void vm_say(const char *value) {
    VMQbit *q = find_qbit(value);
    if (q) {
        print_qbit(q);
        return;
    }

    VMVar *v = find_var(value);
    if (v) {
        printf("%g\n", v->value);
        return;
    }

    printf("%s\n", value);
}

void run_bytecode(Instruction *code, int count) {
    srand(time(NULL));

    printf("=== QBIT NOVA BYTECODE VM ===\n");

    for (int pc = 0; pc < count; pc++) {
        Instruction ins = code[pc];

        switch (ins.op) {
            case OP_SAY:
                vm_say(ins.arg1);
                break;

            case OP_LET:
                set_var(ins.arg1, atof(ins.arg2));
                break;

            case OP_QBIT:
                create_qbit(ins.arg1, ins.arg2);
                break;

            case OP_HADAMARD: {
                VMQbit *q = find_qbit(ins.arg1);
                if (q) hadamard(q);
                else printf("[VM ERROR] qbit not found: %s\n", ins.arg1);
                break;
            }

            case OP_MEASURE: {
                VMQbit *q = find_qbit(ins.arg1);
                if (q) measure(q);
                else printf("[VM ERROR] qbit not found: %s\n", ins.arg1);
                break;
            }

            case OP_CHECK:
                printf("[VM TODO] OP_CHECK not implemented yet\n");
                break;

            case OP_REPEAT:
                printf("[VM TODO] OP_REPEAT not implemented yet\n");
                break;

            case OP_BLOCK:
                break;

            case OP_END:
                break;

            case OP_SAFE_ACTION:
                printf("[VM TODO] OP_SAFE_ACTION not implemented yet\n");
                break;

            default:
                printf("[VM ERROR] unknown opcode: %d\n", ins.op);
                return;
        }
    }
}

#ifdef BYTE_VM_STANDALONE
int main() {
    Instruction program[] = {
        { OP_SAY, "QBIT NOVA BYTE VM Test", "", 0 },
        { OP_LET, "x", "10", 0 },
        { OP_SAY, "x", "", 0 },
        { OP_QBIT, "q", "|0>", 0 },
        { OP_SAY, "q", "", 0 },
        { OP_HADAMARD, "q", "", 0 },
        { OP_SAY, "q", "", 0 },
        { OP_MEASURE, "q", "", 0 },
        { OP_SAY, "q", "", 0 },
        { OP_END, "", "", 0 }
    };

    int count = sizeof(program) / sizeof(program[0]);
    run_bytecode(program, count);
    return 0;
}
#endif
