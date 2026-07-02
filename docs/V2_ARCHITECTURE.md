# QBIT NOVA C v2.0 Architecture

QBIT NOVA C is a layered quantum-style software experiment.

## Layer 1: Classical Processor

QBIT NOVA runs on normal CPUs such as Raspberry Pi or mobile Linux environments.

## Layer 2: QN Source Language

Users write .qn source files.

Example:

    qbit q = |0>
    qbit p = |0>
    hadamard q
    cnot q p
    measure q
    measure p

## Layer 3: Lexer and Parser

The lexer converts source text into tokens.
The parser converts tokens into AST nodes.

## Layer 4: Compiler and Bytecode

The compiler converts AST nodes into QBIT NOVA bytecode instructions.

## Layer 5: Bytecode VM

The bytecode VM executes normal QBIT NOVA instructions, qbits, QMSG, checks, repeats, and safe actions.

## Layer 6: State-vector VM

The STATE2 engine simulates 2-qbit quantum state vectors.

It supports Bell-state simulation:

    H(q)
    CNOT(q, p)

Expected correlated output:

    |00> or |11>

## Layer 7: QMSG / QCPU Virtual Layer

QMSG converts a message into bits and displays them as a virtual qbit register.

Example:

    qmsg "HI"
    encode qmsg
    say qmsg
    measure qmsg
    decode qmsg

## Layer 8: OpenQASM Export

QBIT NOVA can export compatible circuits to OpenQASM 3.0.

Example output:

    OPENQASM 3.0;
    include "stdgates.inc";
    qubit[2] q;
    bit[2] c;
    h q[0];
    cx q[0], q[1];
    c[0] = measure q[0];
    c[1] = measure q[1];

## Safety boundary

This project is a software simulation and virtual processor layer.
It does not convert classical hardware into physical quantum hardware.
