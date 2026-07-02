# QCPU Fault Memory

QCPU Fault Memory is the first local memory layer for QBIT NOVA C fault and recovery events.

It records what failed, what was detected, how recovery happened, and whether the runtime returned to a safe state.

## Purpose

This layer proves:

- fault/noise events can be remembered
- recovery status can be recorded
- local proof history can be generated
- QBIT NOVA can keep a non-destructive recovery trail

## Rule

Memory is local and software-only.

No hardware mutation, kernel patching, voltage control, or destructive system action is performed.

## Run

    ./scripts/qcpu_fault_memory.sh

## Outputs

    logs/qcpu_fault_memory.log
    build/qcpu_fault_memory.md

## Meaning

This is the memory seed after detection and recovery.

QBIT NOVA can now say:

    I saw noise.
    I entered recovery.
    I re-ran clean proof.
    I rebooted virtual QCPU.
    I kept the proof trail.

## Safety boundary

Fault Memory records events.

It does not change the core engine.
