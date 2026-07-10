# QCPU Circuit VM

QCPU Circuit VM is a C-based software virtual QCPU circuit layer for QBIT NOVA C.

It runs on classical hardware such as Raspberry Pi 5.

It is not physical quantum hardware.

## Supported gates

- h
- x
- y
- z
- s
- t
- cx
- swap

## Macro

- ghz

The ghz macro expands into:

- h q0
- cx q0 q1
- cx q1 q2
- continues as a CNOT chain for N qubits

## Example circuit

examples/ghz3.qnc:

qubits 3
ghz
measure

## Run

Compile and run through the proof script:

./scripts/proof_qcircuit.sh

## OpenQASM export

The Circuit VM can emit OpenQASM 3.0.

Example:

OPENQASM 3.0;
include "stdgates.inc";

qubit[3] q;
bit[3] c;

h q[0];
cx q[0], q[1];
cx q[1], q[2];

c[0] = measure q[0];
c[1] = measure q[1];
c[2] = measure q[2];

## Safety boundary

This is a software virtual QCPU proof.

It validates circuit-level state-vector behavior and OpenQASM export.

It does not claim Raspberry Pi became physical quantum hardware.
