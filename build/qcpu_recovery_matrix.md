# QCPU Recovery Matrix

Generated UTC: 2026-07-20T10:30:41Z

## Host

- Host: nova-pi
- Architecture: aarch64
- Kernel: 6.18.34+rpt-rpi-2712

## Recovery Results

| Phase | Result | Meaning |
|---|---|---|
| Noise detection | PASS: NOISE_DETECTED | Noisy sample was detected |
| Recovery mode | PASS: RECOVERY_MODE_ACTIVE | Recovery state was entered |
| Clean proof re-run | PASS: CLEAN_BELL_PROOF_AFTER_RECOVERY | Bell proof remains valid after recovery |
| Virtual QCPU reboot | PASS: VIRTUAL_QCPU_REBOOTED | Virtual QCPU restarted safely |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_RECOVERY | Core engine was not modified |
| Safety boundary | PASS: NON_DESTRUCTIVE_SOFTWARE_RECOVERY | No destructive action was performed |

## Recovery State

```text
QCPU_RECOVERY_MODE=ACTIVE
QCPU_RECOVERY_REASON=NOISY_BELL_OUTPUT_FOUND
QCPU_RECOVERY_TIME_UTC=2026-07-20T10:30:41Z
QCPU_RECOVERY_ACTION=RERUN_CLEAN_PROOF_AND_REBOOT_VIRTUAL_QCPU
```

## Clean Bell Proof After Recovery

```text
bad count: 0
BELL PROOF PASSED
```

## Boundary Statement

Noise was detected first.

    PASS: NOISE_DETECTED

Recovery mode started.

    PASS: RECOVERY_MODE_ACTIVE

Clean proof passed after recovery.

    PASS: CLEAN_BELL_PROOF_AFTER_RECOVERY

Virtual QCPU rebooted safely.

    PASS: VIRTUAL_QCPU_REBOOTED

## Verdict

QCPU RECOVERY MATRIX READY
