# NOVA Hypercube Runtime

NOVA Hypercube Runtime is the name for the QBIT NOVA software world where a normal Raspberry Pi or mobile Linux device runs as a virtual QCPU node.

## Core idea

Normal world:

    classical CPU
    Linux
    normal commands

NOVA Hypercube world:

    classical CPU
    Linux
    QBIT NOVA runtime
    virtual QCPU session
    Bell proof
    QASM export
    QMSG virtual register

## Important boundary

This does not claim that a phone, Raspberry Pi, or CPU becomes physical quantum hardware.

It means the device enters a software-defined quantum-style runtime layer.

## Current foundation

Built from QBIT NOVA C:

- v2.0 public release checkpoint
- v2.1 QASM file export bridge
- v2.2 virtual QCPU boot layer

## Runtime identity

A NOVA Hypercube node proves itself by creating:

    .qcpu/session.env
    build/qcpu_node.json
    build/bell.qasm
    logs/qcpu_boot.log

## Proof chain

The runtime must pass:

    ./scripts/proof_bell.sh 10
    ./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm
    ./scripts/qcpu_boot.sh

Expected proof:

    BELL PROOF PASSED
    Only |00> and |11> appeared.
    QCPU NODE READY

## Meaning

NOVA Hypercube Runtime is not fake hardware.

It is a safe virtual execution world:

    hardware body + QBIT NOVA quantum-style brain layer
