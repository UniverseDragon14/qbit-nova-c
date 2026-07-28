# QCPU Hardware Reality Probe

Generated UTC: 2026-07-20T10:30:43Z

## Host Reality

| Field | Value |
|---|---|
| Host | nova-pi |
| OS | Debian GNU/Linux 13 (trixie) |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| CPU model | Raspberry Pi 5 Model B Rev 1.0 |
| Temperature | 56.5'C |
| Temperature status | PASS: HARDWARE_TEMP_SAFE |
| Throttle value | 0x0 |
| Throttle status | PASS: NO_THROTTLING |
| Memory total | 7.9Gi |
| Memory free | 3.8Gi |
| Disk root | 113G total, 66G used, 43G free, 61% used |
| Uptime |  14:30:43 up  7:06,  3 users,  load average: 0.22, 0.09, 0.03 |

## QCPU Boundary

| Check | Result | Meaning |
|---|---|---|
| Physical QCPU device | EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND | Honest physical hardware check |
| Virtual QCPU support | PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST | Classical host can run software QCPU runtime |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_HARDWARE_PROBE | Core engine was not modified |
| Safety boundary | PASS: READ_ONLY_HARDWARE_PROBE | Read-only hardware probe only |

## Boundary Statement

Physical QCPU check:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

Virtual QCPU check:

    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST

Temperature check:

    PASS: HARDWARE_TEMP_SAFE

Throttle check:

    PASS: NO_THROTTLING

Safety check:

    PASS: READ_ONLY_HARDWARE_PROBE

## Verdict

QCPU HARDWARE REALITY PROBE READY
