# QBIT NOVA C v2.0 Release Notes

QBIT NOVA C v2.0 is the first public release checkpoint for the QBIT NOVA C project.

Creator: Universal Dragon Aslam

## What QBIT NOVA C is

QBIT NOVA C is a small C-based quantum-style language, bytecode VM, state-vector simulator, QMSG/QCPU virtual layer, OpenQASM exporter, and Bell-state proof project.

It runs on classical hardware such as Raspberry Pi and Termux-style Linux environments.

## Important safety statement

QBIT NOVA C does not claim that a normal phone, Raspberry Pi, or classical CPU becomes a physical quantum computer.

It builds a software virtual quantum processor layer on top of classical hardware.

## Included by v2.0

- .qn language syntax
- Lexer, parser, AST, compiler
- Bytecode VM
- Unified qnova runner
- Qbit simulation
- Hadamard gate
- CNOT simulation
- Entangle simulation
- Repeat blocks
- Safe action adapter contract
- 2-qbit state-vector engine
- QMSG virtual packet layer
- QMSG virtual qbit register view
- OpenQASM 3.0 exporter
- Bell-state correlation proof
- Public proof pack
- GitHub CI workflow
- CI badge
- Quick demo script

## Main proof commands

    ./scripts/test_all.sh
    ./scripts/demo.sh
    ./scripts/proof_bell.sh 20

## v2.0 meaning

QBIT NOVA C has moved from experiment-only code into a documented, testable, public-demo-ready release checkpoint.
