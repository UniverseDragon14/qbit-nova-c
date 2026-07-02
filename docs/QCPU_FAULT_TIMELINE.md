# QCPU Fault Timeline

QCPU Fault Timeline converts QCPU Fault Memory events into a readable timeline.

## Purpose

This layer proves:

- multiple fault memory events can be collected
- the latest recovery trail can be summarized
- fault history can be inspected without mutating the core engine
- QBIT NOVA can explain what happened over time

## Rule

Timeline is software-only.

No hardware mutation, kernel patching, voltage control, or destructive system action is performed.

## Run

    ./scripts/qcpu_fault_timeline.sh

## Outputs

    build/qcpu_fault_timeline.md
    logs/qcpu_fault_memory.log

## Meaning

QBIT NOVA can now say:

    I detected faults.
    I recovered.
    I remembered them.
    I can show the timeline.

## Safety boundary

Fault Timeline reads and summarizes local logs.

It does not change the core engine.
