# QCPU Noise Injection Matrix

The QCPU Noise Injection Matrix is a controlled failure test for QBIT NOVA C.

It does not damage the simulator.

It injects a synthetic noisy result into a report and verifies that the system can detect it.

## Purpose

This layer tests:

- clean Bell proof baseline
- synthetic noisy quantum-style result
- bad outcome detection
- honest reporting
- safe failure handling

## Rule

Noise is injected into the test report only.

The QBIT NOVA core engine is not mutated.

## Expected results

| Test | Expected Result | Meaning |
|---|---|---|
| Clean Bell proof | PASS | Bell simulation remains correct |
| Synthetic noisy sample | EXPECTED_DETECT | Bad output is detected |
| Core mutation check | PASS | No core files are changed |
| Safety boundary | PASS | No hardware or destructive action |

## Run

    ./scripts/qcpu_noise_injection.sh

## Output

    build/qcpu_noise_injection.md

## Meaning

This is the first noise/fault-tolerance style test layer.

A bad result is allowed to appear only inside the controlled sample.

The detector must not pretend it is valid.
