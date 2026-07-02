# NOVA Hypercube Snapshot Report

The NOVA Hypercube Snapshot Report creates a local proof summary for the current QBIT NOVA virtual runtime.

## Purpose

The snapshot records:

- QCPU node identity
- Runtime mode
- Safety boundary
- Bell proof result
- QASM export proof
- Git version tags

## Run

    ./scripts/hypercube_snapshot.sh

## Output

    build/hypercube_snapshot.md

## Meaning

This is a local runtime receipt.

It proves that the device entered the NOVA Hypercube Runtime layer and passed the required QBIT NOVA checks.

## Safety boundary

This is still a software-defined runtime layer on classical hardware.
It is not physical quantum hardware.
