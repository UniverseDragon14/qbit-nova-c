# QCPU Recovery Matrix

The QCPU Recovery Matrix is the first recovery layer for QBIT NOVA C.

It tests what happens after a noisy or invalid quantum-style result is detected.

## Purpose

This layer proves:

- noise can be detected
- recovery mode can start
- clean Bell proof can be re-run
- virtual QCPU can reboot safely
- recovery status can be reported honestly

## Rule

Recovery is software-only.

No hardware mutation, kernel patching, voltage control, or destructive action is performed.

## Expected results

| Test | Expected Result | Meaning |
|---|---|---|
| Noise detection | PASS | Invalid Bell output is detected |
| Recovery mode | PASS | Recovery state is entered |
| Clean proof re-run | PASS | Bell proof is valid after recovery |
| Virtual QCPU reboot | PASS | QCPU runtime can restart after detection |
| Core mutation check | PASS | Core engine remains unchanged |

## Run

    ./scripts/qcpu_recovery_matrix.sh

## Output

    build/qcpu_recovery_matrix.md

## Meaning

This layer is the first seed of self-healing behavior.

The runtime does not pretend noise is valid.

It detects the noisy sample, enters recovery, re-runs clean proof, and confirms the virtual runtime is safe.
