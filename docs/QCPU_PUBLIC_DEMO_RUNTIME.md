# QCPU Public Demo Runtime

QCPU Public Demo Runtime is the v4.0 public demonstration layer for QBIT NOVA C.

It gives a single safe public proof flow:

- verify public release manifest
- verify release readiness seal
- verify CI evidence chain
- boot virtual QCPU
- run Bell proof
- export OpenQASM
- create public demo receipt

## Boundary

This is software-only.

It does not mutate hardware.

It does not claim that Raspberry Pi or any classical device became physical quantum hardware.

The physical QCPU check remains an honest expected-fail.

The virtual QCPU path is the supported runtime path.

## Expected verdict

QCPU PUBLIC DEMO RUNTIME READY
PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY
