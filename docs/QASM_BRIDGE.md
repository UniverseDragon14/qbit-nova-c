# QBIT NOVA C - QASM File Export Bridge

QBIT NOVA C can export `.qn` quantum-style source into OpenQASM 3.0 style output.

## Purpose

This is the bridge from QBIT NOVA language into the wider quantum software ecosystem.

QBIT NOVA source:

    qbit q = |0>
    qbit p = |0>
    hadamard q
    cnot q p
    measure q
    measure p

Exported QASM:

    OPENQASM 3.0;
    include "stdgates.inc";

    qubit[2] q;
    bit[2] c;

    h q[0];
    cx q[0], q[1];
    c[0] = measure q[0];
    c[1] = measure q[1];

## Export command

Run:

    ./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm

Then view:

    cat build/bell.qasm

## Meaning

v2.1 creates a reusable file-based bridge:

    .qn source
    -> QBIT NOVA compiler
    -> OpenQASM text file
    -> future simulator or real quantum backend

## Safety boundary

This is software export.
It does not claim local hardware becomes physical quantum hardware.
