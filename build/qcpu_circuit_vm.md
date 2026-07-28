# QCPU Circuit VM Proof

Generated UTC: 2026-07-20T10:30:49Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Circuit VM Result

| Field | Value |
|---|---|
| Example | examples/ghz3.qnc |
| Measured result | |000> |
| Circuit status | PASS: QCIRCUIT_GHZ_MEASUREMENT_VALID |
| OpenQASM export | PASS: QCIRCUIT_OPENQASM_EXPORT_READY |
| Decision | ALLOW_CIRCUIT_VM |
| Status | PASS: QCPU_CIRCUIT_VM_READY |

## Supported Gates

- h
- x
- y
- z
- s
- t
- cx
- swap
- ghz macro

## Truth Boundary

This is a C-based software virtual QCPU circuit proof.

It runs on classical hardware such as Raspberry Pi 5.

It does not claim physical quantum hardware.

It validates circuit-level software state-vector behavior and OpenQASM export.

## Verdict

PASS: QCPU_CIRCUIT_VM_READY
