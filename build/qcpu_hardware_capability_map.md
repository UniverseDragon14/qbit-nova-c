# QCPU Hardware Capability Map

Generated UTC: 2026-07-20T10:30:43Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| OS | Debian GNU/Linux 13 (trixie) |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| CPU model | Raspberry Pi 5 Model B Rev 1.0 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Hardware Capability

| Capability | Value | Status |
|---|---|---|
| CPU cores | 4 | PASS: CPU_CORE_CAPACITY_OK |
| RAM total | 8062 MB | PASS: RAM_CAPACITY_OK |
| RAM available | 6589 MB | PASS: RAM_AVAILABLE_OK |
| Disk free | 43403 MB | PASS: DISK_CAPACITY_OK |
| Disk used | 61% | PASS: DISK_USAGE_RECORDED |
| Temperature | 57.1'C | PASS: THERMAL_CAPACITY_OK |
| Throttle | 0x0 | PASS: THROTTLE_CLEAR |

## Tool Capability

| Tool | Status |
|---|---|
| gcc | PASS: GCC_AVAILABLE |
| python3 | PASS: PYTHON3_AVAILABLE |
| git | PASS: GIT_AVAILABLE |

## QCPU Runtime Recommendation

| Field | Value |
|---|---|
| Recommended mode | STANDARD_VIRTUAL_QCPU_MODE |
| Mode status | PASS: STANDARD_MODE_RECOMMENDED |
| Core status | PASS: CORE_NOT_MUTATED_BY_CAPABILITY_MAP |
| Safety status | PASS: READ_ONLY_CAPABILITY_MAP |

## Hardware Reality Probe Summary

```text
EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND
PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST
PASS: READ_ONLY_HARDWARE_PROBE
| Physical QCPU device | EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND | Honest physical hardware check |
| Virtual QCPU support | PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST | Classical host can run software QCPU runtime |
| Safety boundary | PASS: READ_ONLY_HARDWARE_PROBE | Read-only hardware probe only |
    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND
    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST
    PASS: READ_ONLY_HARDWARE_PROBE
QCPU HARDWARE REALITY PROBE READY
QCPU HARDWARE REALITY PROBE READY
```

## Boundary Statement

This capability map is read-only.

It does not convert classical hardware into physical quantum hardware.

Physical QCPU remains:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

Virtual QCPU capability is:

    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST

Recommended runtime mode:

    STANDARD_VIRTUAL_QCPU_MODE

Safety:

    PASS: READ_ONLY_CAPABILITY_MAP

## Verdict

QCPU HARDWARE CAPABILITY MAP READY
