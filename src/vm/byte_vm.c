#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "bytecode.h"
#include "../adapter/safe_adapter.h"

#define MAX_VM_VARS 64
#define MAX_VM_QBITS 32
#define QMSG_MAX_CHARS 128
#define QMSG_MAX_BITS (QMSG_MAX_CHARS * 8 + 1)

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
    char entangled_with[64];
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

static void qmsg_print_register(void);

static void vm_say(const char *value) {
    if (strcmp(value, "qmsg") == 0) {
        qmsg_print_register();
        return;
    }

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

static void cnot_qbits(const char *control_name, const char *target_name) {
    VMQbit *control = find_qbit(control_name);
    VMQbit *target = find_qbit(target_name);

    if (!control || !target) {
        printf("[QCORE ERROR] cannot CNOT missing qbit: %s %s\n", control_name, target_name);
        return;
    }

    strcpy(control->entangled_with, target_name);
    strcpy(target->entangled_with, control_name);

    target->a = control->a;
    target->b = control->b;
    target->measured = 0;
    target->result = 0;

    printf("[QCORE] CNOT %s -> %s\n", control_name, target_name);
}

static void entangle_qbits(const char *left_name, const char *right_name) {
    VMQbit *left = find_qbit(left_name);
    VMQbit *right = find_qbit(right_name);

    if (!left || !right) {
        printf("[QCORE ERROR] cannot entangle missing qbit: %s %s\n", left_name, right_name);
        return;
    }

    strcpy(left->entangled_with, right_name);
    strcpy(right->entangled_with, left_name);

    right->a = left->a;
    right->b = left->b;
    right->measured = 0;
    right->result = 0;

    printf("[QCORE] entangled %s <-> %s\n", left_name, right_name);
}

static void measure_linked(VMQbit *q) {
    measure(q);

    if (q->entangled_with[0]) {
        VMQbit *partner = find_qbit(q->entangled_with);
        if (partner) {
            partner->result = q->result;
            partner->measured = 1;

            if (q->result == 0) {
                partner->a = 1.0;
                partner->b = 0.0;
            } else {
                partner->a = 0.0;
                partner->b = 1.0;
            }

            printf("[QCORE] entangled collapse %s -> %s = |%d>\n", q->name, partner->name, q->result);
        }
    }
}


static char qmsg_text[QMSG_MAX_CHARS];
static char qmsg_bits[QMSG_MAX_BITS];
static int qmsg_loaded = 0;
static int qmsg_encoded = 0;
static int qmsg_measured = 0;

static void qmsg_set(const char *message) {
    if (!message || !message[0]) {
        printf("[QMSG ERROR] empty message\n");
        return;
    }

    strncpy(qmsg_text, message, QMSG_MAX_CHARS - 1);
    qmsg_text[QMSG_MAX_CHARS - 1] = '\0';
    qmsg_bits[0] = '\0';

    qmsg_loaded = 1;
    qmsg_encoded = 0;
    qmsg_measured = 0;

    printf("[QMSG] message loaded: %s\n", qmsg_text);
}

static void qmsg_encode(void) {
    if (!qmsg_loaded) {
        printf("[QMSG ERROR] no qmsg loaded\n");
        return;
    }

    int bit_pos = 0;

    for (int i = 0; qmsg_text[i] && bit_pos < QMSG_MAX_BITS - 9; i++) {
        unsigned char ch = (unsigned char)qmsg_text[i];

        for (int b = 7; b >= 0; b--) {
            qmsg_bits[bit_pos++] = ((ch >> b) & 1) ? '1' : '0';
        }
    }

    qmsg_bits[bit_pos] = '\0';
    qmsg_encoded = 1;
    qmsg_measured = 0;

    printf("[QMSG] bits: %s\n", qmsg_bits);
    printf("[QMSG] virtual qbit packet created: %d bits\n", bit_pos);
}

static void qmsg_measure(void) {
    if (!qmsg_encoded) {
        printf("[QMSG ERROR] qmsg must be encoded before measure\n");
        return;
    }

    qmsg_measured = 1;
    printf("[QMSG] measured virtual qbit packet: %zu bits\n", strlen(qmsg_bits));
}


static void qmsg_print_register(void) {
    if (!qmsg_encoded) {
        printf("[QMSG ERROR] qmsg must be encoded before register view\n");
        return;
    }

    size_t len = strlen(qmsg_bits);

    printf("[QMSG] virtual qbit register:\n");

    for (size_t i = 0; i < len; i++) {
        printf("q%zu=|%c>", i, qmsg_bits[i]);

        if ((i + 1) % 8 == 0 || i + 1 == len) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
}

static void qmsg_decode(void) {
    if (!qmsg_encoded) {
        printf("[QMSG ERROR] qmsg must be encoded before decode\n");
        return;
    }

    if (!qmsg_measured) {
        printf("[QMSG WARN] decoding before measure\n");
    }

    printf("[QMSG] decoded message: %s\n", qmsg_text);
}


static void run_safe_action(const char *action) {
    AdapterResult result = adapter_dispatch_safe(action);

    if (result == ADAPTER_OK) {
        printf("[SAFE_ACTION] dispatched through adapter contract: %s\n", action);
    }
}

void run_bytecode(Instruction *code, int count) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    printf("=== QBIT NOVA BYTECODE VM ===\n");

    int repeat_start[64];
    int repeat_end[64];
    int repeat_left[64];
    int repeat_sp = 0;

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

            case OP_CNOT:
                cnot_qbits(ins.arg1, ins.arg2);
                break;

            case OP_ENTANGLE:
                entangle_qbits(ins.arg1, ins.arg2);
                break;

            case OP_MEASURE: {
                if (strcmp(ins.arg1, "qmsg") == 0) {
                    qmsg_measure();
                    break;
                }

                VMQbit *q = find_qbit(ins.arg1);
                if (q) measure_linked(q);
                else printf("[VM ERROR] qbit not found: %s\n", ins.arg1);
                break;
            }

            case OP_CHECK: {
                VMVar *v = find_var(ins.arg1);
                int pass = 0;

                if (!v) {
                    printf("[VM ERROR] variable not found for check: %s\n", ins.arg1);
                } else {
                    double lhs = v->value;
                    double rhs = ins.value;

                    if (strcmp(ins.arg2, ">") == 0) pass = lhs > rhs;
                    else if (strcmp(ins.arg2, "<") == 0) pass = lhs < rhs;
                    else if (strcmp(ins.arg2, "==") == 0) pass = lhs == rhs;
                    else if (strcmp(ins.arg2, ">=") == 0) pass = lhs >= rhs;
                    else if (strcmp(ins.arg2, "<=") == 0) pass = lhs <= rhs;
                    else printf("[VM ERROR] unknown check operator: %s\n", ins.arg2);
                }

                if (!pass) {
                    int depth = 0;
                    while (pc + 1 < count) {
                        pc++;
                        if (code[pc].op == OP_CHECK || code[pc].op == OP_REPEAT) depth++;
                        if (code[pc].op == OP_END) {
                            if (depth == 0) break;
                            depth--;
                        }
                    }
                }
                break;
            }

            case OP_REPEAT: {
                int end = -1;
                int depth = 0;

                for (int i = pc + 1; i < count; i++) {
                    if (code[i].op == OP_CHECK || code[i].op == OP_REPEAT) depth++;

                    if (code[i].op == OP_END) {
                        if (depth == 0) {
                            end = i;
                            break;
                        }
                        depth--;
                    }
                }

                if (end == -1) {
                    printf("[VM ERROR] repeat block missing END\n");
                    return;
                }

                if (ins.value <= 0) {
                    pc = end;
                    break;
                }

                if (repeat_sp >= 64) {
                    printf("[VM ERROR] repeat stack overflow\n");
                    return;
                }

                repeat_start[repeat_sp] = pc;
                repeat_end[repeat_sp] = end;
                repeat_left[repeat_sp] = ins.value;
                repeat_sp++;
                break;
            }

            case OP_BLOCK:
                break;

            case OP_END:
                if (repeat_sp > 0 && pc == repeat_end[repeat_sp - 1]) {
                    repeat_left[repeat_sp - 1]--;

                    if (repeat_left[repeat_sp - 1] > 0) {
                        pc = repeat_start[repeat_sp - 1];
                    } else {
                        repeat_sp--;
                    }
                }
                break;

            case OP_QMSG:
                qmsg_set(ins.arg1);
                break;

            case OP_ENCODE_QMSG:
                if (strcmp(ins.arg1, "qmsg") == 0) qmsg_encode();
                else printf("[QMSG ERROR] unknown encode target: %s\n", ins.arg1);
                break;

            case OP_DECODE_QMSG:
                if (strcmp(ins.arg1, "qmsg") == 0) qmsg_decode();
                else printf("[QMSG ERROR] unknown decode target: %s\n", ins.arg1);
                break;

            case OP_SAFE_ACTION:
                run_safe_action(ins.arg1);
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
