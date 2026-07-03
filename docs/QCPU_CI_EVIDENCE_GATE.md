# QCPU CI Evidence Gate

v3.7 adds the QCPU CI Evidence Gate.

The gate checks v3.6 CI Evidence Reporter output and opens only when required safety markers are present.

## Required Markers

| Gate | Required marker |
|---|---|
| Reporter ready | QCPU CI EVIDENCE REPORTER READY |
| CI fallback ready | PASS: CI_SAFE_FALLBACK_READY |
| Physical QCPU honesty | EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND |
| Virtual QCPU support | PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST |
| Core safety | PASS: CORE_NOT_MUTATED_BY_CI_EVIDENCE_REPORTER |
| Non destructive boundary | PASS: NON_DESTRUCTIVE_CI_EVIDENCE_REPORTER |

## Boundary Statement

The CI Evidence Gate is software-only.

It checks evidence. It does not mutate hardware.

Physical QCPU remains:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

Virtual QCPU support remains:

    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST

Gate result must be:

    PASS: CI_EVIDENCE_GATE_OPENED

## Verdict

QCPU CI EVIDENCE GATE READY
