# QBIT NOVA C

[![QBIT NOVA CI](https://github.com/UniverseDragon14/qbit-nova-c/actions/workflows/qbit-nova-ci.yml/badge.svg)](https://github.com/UniverseDragon14/qbit-nova-c/actions/workflows/qbit-nova-ci.yml)


Creator: Universal Dragon Aslam
Project: QBIT NOVA C

QBIT NOVA C is a small C-based quantum-style language, bytecode VM, state-vector simulator, and virtual QMSG/QCPU layer.

Important safety note:
QBIT NOVA C does not claim that a normal phone or Raspberry Pi processor becomes a physical quantum chip.
It builds a software virtual quantum processor layer on top of classical hardware.

## Current Capabilities

- .qn language syntax
- Lexer, parser, AST, compiler
- Bytecode VM
- Safe action adapter contract
- Qbit simulation
- Hadamard gate
- CNOT simulation
- Entangle simulation
- Repeat blocks
- 2-qbit state-vector core
- Unified qnova runner
- QMSG virtual packet layer
- QMSG virtual qbit register view
- OpenQASM exporter

## Build

Run:

    gcc src/qnova.c \
        src/lexer/lexer.c \
        src/parser/parser.c \
        src/compiler/compiler.c \
        src/vm/byte_vm.c \
        src/vm/state2_vm.c \
        src/quantum/state2.c \
        src/adapter/safe_adapter.c \
        -o qnova -lm

## Run Examples

    ./qnova examples/full_test.qn
    ./qnova examples/bell_state2.qn
    ./qnova examples/qmsg_hi.qn
    ./qnova examples/qmsg_register.qn

## QMSG Register Proof

Source idea:

    qmsg "HI"
    encode qmsg
    say qmsg
    measure qmsg
    decode qmsg

Expected output:

    [QMSG] bits: 0100100001001001
    [QMSG] virtual qbit register:
    q0=|0> q1=|1> q2=|0> q3=|0> q4=|1> q5=|0> q6=|0> q7=|0>
    q8=|0> q9=|1> q10=|0> q11=|0> q12=|1> q13=|0> q14=|0> q15=|1>
    [QMSG] decoded message: HI

## Version Timeline

| Version | Feature |
|---|---|
| v1.0 | 2-qbit state-vector core |
| v1.1 | .qn to state-vector pipeline |
| v1.2 | Unified qnova runner |
| v1.3 | QMSG virtual packet layer |
| v1.4 | QMSG virtual qbit register view |
| v1.5 | Public proof pack |
| v1.6 | GitHub CI proof tests |
| v1.7 | CI status badge |
| v1.8 | OpenQASM export |

## Vision

QBIT NOVA is a seven-layer experiment:

1. Classical processor
2. QBIT packet layer
3. Virtual QPU
4. State-vector brain
5. QMSG message layer
6. Pi5 quantum node
7. Future real QPU bridge through OpenQASM


## OpenQASM Export

Build exporter:

    gcc src/tools/qasm_export.c \
        src/lexer/lexer.c \
        src/parser/parser.c \
        src/compiler/compiler.c \
        -o qnova-qasm -lm

Run:

    ./qnova-qasm examples/bell_qasm.qn

Expected output includes:

    OPENQASM 3.0;
    include "stdgates.inc";
    qubit[2] q;
    bit[2] c;
    h q[0];
    cx q[0], q[1];
    c[0] = measure q[0];
    c[1] = measure q[1];


## Bell State Proof

Run:

    ./scripts/proof_bell.sh 20

Expected result:

    Only |00> and |11> appeared.

This proves the QBIT NOVA state-vector engine preserves Bell-state measurement correlation.
