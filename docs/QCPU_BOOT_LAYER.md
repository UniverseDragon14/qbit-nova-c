# QBIT NOVA C - Virtual QCPU Boot Layer

The Virtual QCPU Boot Layer is a safe software runtime identity layer.

It does not convert a Raspberry Pi, phone, or CPU into physical quantum hardware.

Instead, it lets classical hardware operate inside a QBIT NOVA quantum-style runtime context.

## Concept

Normal mode:

    CPU -> Linux -> normal commands

QCPU mode:

    CPU -> Linux -> QBIT NOVA runtime -> QN / QASM / QMSG / Bell proof

## What it creates

    .qcpu/session.env
    build/qcpu_node.json
    build/bell.qasm
    logs/qcpu_boot.log
    logs/qcpu_bell.log
    logs/qcpu_qasm_export.log

## Run

    ./scripts/qcpu_boot.sh

## Meaning

This is the first step toward making a normal Pi5 or mobile Linux device behave as a virtual QCPU node.

The processor is still classical hardware.
The quantum behavior is implemented in software through QBIT NOVA C.

## Safety boundary

This project does not touch voltage, firmware, unsafe kernel controls, cryogenic systems, lasers, or physical quantum hardware.

It is a software VM/runtime experiment.
