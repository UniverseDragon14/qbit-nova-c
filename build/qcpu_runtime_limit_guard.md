# QCPU Runtime Limit Guard

Generated UTC: 2026-07-26T11:17:16Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| CPU cores | 4 |
| Commit | 87627a7 docs: finalize Devpost submission evidence |
| CI env | LOCAL_TERMINAL |

## Runtime Limits

| Limit | Value |
|---|---|
| Runtime mode | STANDARD_VIRTUAL_QCPU_MODE |
| Max shots | 100 |
| Max proof loops | 20 |
| Max parallel jobs | 1 |
| Temperature | 53.2'C |
| Throttle | 0x0 |
| vcgencmd | PASS: VCGENCMD_AVAILABLE |
| Memory free MB | 6778 |
| Disk free MB | 41742 |

## Guard Status

| Check | Result |
|---|---|
| Guard | PASS: STANDARD_RUNTIME_LIMITS_ACTIVE |
| Temperature | PASS: TEMP_SAFE_FOR_RUNTIME |
| Throttle | PASS: NO_THROTTLING |
| Memory | PASS: MEMORY_AVAILABLE_FOR_RUNTIME |
| Disk | PASS: DISK_AVAILABLE_FOR_RUNTIME |
| Core | PASS: CORE_NOT_MUTATED_BY_RUNTIME_LIMIT_GUARD |
| Safety | PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD |

## Boundary Statement

The runtime guard is software-only.

It does not mutate hardware.

On Raspberry Pi, it reads vcgencmd when available.

On GitHub Actions or CI without vcgencmd, it uses a CI-safe fallback.

The CI fallback does not claim physical Raspberry Pi hardware.

It only allows the virtual QCPU proof chain to run in a classical CI runner.

Recommended runtime mode:

    STANDARD_VIRTUAL_QCPU_MODE

Guard status:

    PASS: STANDARD_RUNTIME_LIMITS_ACTIVE

Safety:

    PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD

## Verdict

QCPU RUNTIME LIMIT GUARD READY
