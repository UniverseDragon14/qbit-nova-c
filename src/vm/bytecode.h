#ifndef QBIT_NOVA_BYTECODE_H
#define QBIT_NOVA_BYTECODE_H

#define MAX_INSTR 1024

typedef enum {
    OP_SAY,
    OP_LET,
    OP_QBIT,
    OP_HADAMARD,
    OP_MEASURE,
    OP_CHECK,
    OP_REPEAT,
    OP_BLOCK,
    OP_END,
    OP_SAFE_ACTION
} OpCode;

typedef enum {
    ACTION_LED_ON,
    ACTION_LED_OFF,
    ACTION_SERVO_UP,
    ACTION_SERVO_DOWN,
    ACTION_BUZZER_BEEP
} SafeActionCode;

typedef struct {
    OpCode op;
    char arg1[64];
    char arg2[64];
    int value;
} Instruction;

#endif
