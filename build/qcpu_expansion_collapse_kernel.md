# QCPU Expansion-Collapse Kernel

Generated UTC: 2026-07-20T10:30:50Z

## Verified Runtime

| Field | Result |
|---|---|
| Version | v4.5 |
| Mode | STANDARD_VIRTUAL_QCPU_MODE |
| Compact input | 3-qubit GHZ circuit |
| Expanded state count | 8 basis states |
| Normalization | PASS |
| Shot count | 20 |
| Invalid GHZ outcomes | 0 |
| Collapse result | 000 or 111 |

## State

The compact circuit prepares:

    |000> = 0.707107
    |111> = 0.707107

All other basis amplitudes are zero within the tested tolerance.

## Boundary

This is a C software statevector Virtual QCPU.

It does not claim physical quantum hardware.

## Verdict

PASS: QCPU_EXPANSION_COLLAPSE_KERNEL_READY
