# QCPU Boundary Fit Matrix

The QCPU Boundary Fit Matrix is the first controlled failure layer in QBIT NOVA C.

It tests which stones fit, which stones fall, and why.

## Purpose

This layer separates:

- physical quantum hardware claims
- virtual QCPU runtime proof
- QASM bridge readiness
- Bell proof correctness
- NOVA Hypercube runtime identity

## Key rule

Failure is allowed.

Damage is not allowed.

## Boundary result model

| Test | Expected Result | Meaning |
|---|---|---|
| Physical QCPU device | EXPECTED_FAIL | Normal Pi/mobile has no physical quantum chip |
| Virtual QCPU boot | PASS | Software QCPU runtime works |
| Bell proof | PASS | Entanglement simulation works |
| QASM export | PASS | External quantum bridge format works |
| Hypercube status | PASS | Runtime identity is valid |
| Snapshot report | PASS | Runtime proof receipt exists |

## Important honesty boundary

This project does not claim that Raspberry Pi, phones, or classical CPUs become physical quantum hardware.

QBIT NOVA C creates a software-defined quantum-style runtime.

## Run

    ./scripts/qcpu_boundary_fit.sh

## Output

    build/qcpu_boundary_fit.md

## Meaning

A falling stone is data.

If physical QCPU is not found, the result is not hidden.

It is reported honestly as:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

If virtual QCPU works, the report confirms:

    PASS: VIRTUAL_QCPU_READY
