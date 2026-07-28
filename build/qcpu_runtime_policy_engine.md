# QCPU Runtime Policy Engine

Generated UTC: 2026-07-26T11:17:16Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Commit | 87627a7 docs: finalize Devpost submission evidence |

## Runtime Limits Read

| Limit | Value |
|---|---|
| Runtime mode | STANDARD_VIRTUAL_QCPU_MODE |
| Max shots | 100 |
| Max proof loops | 20 |
| Max parallel jobs | 1 |
| Temperature | 53.2'C |
| Throttle | 0x0 |
| Limit guard status | PASS: STANDARD_RUNTIME_LIMITS_ACTIVE |

## Policy Decisions

| Workload | Request | Decision | Status |
|---|---|---|---|
| Standard workload | shots=20 loops=5 | ALLOW_STANDARD_WORKLOAD | PASS: STANDARD_WORKLOAD_ALLOWED |
| Heavy workload | shots=1000 loops=200 | BLOCK_HEAVY_WORKLOAD | PASS: HEAVY_WORKLOAD_BLOCKED |
| Final policy | mode=STANDARD_VIRTUAL_QCPU_MODE | ALLOW_STANDARD_WORKLOAD | PASS: STANDARD_POLICY_ACTIVE |

## Policy Env

```text
QCPU_POLICY_MODE=STANDARD_VIRTUAL_QCPU_MODE
QCPU_STANDARD_DECISION=ALLOW_STANDARD_WORKLOAD
QCPU_HEAVY_DECISION=BLOCK_HEAVY_WORKLOAD
QCPU_FINAL_POLICY=ALLOW_STANDARD_WORKLOAD
QCPU_STANDARD_SHOTS=20
QCPU_STANDARD_LOOPS=5
QCPU_HEAVY_SHOTS=1000
QCPU_HEAVY_LOOPS=200
QCPU_POLICY_CREATED_UTC=2026-07-26T11:17:16Z
QCPU_POLICY_STATUS=PASS: STANDARD_POLICY_ACTIVE
```

## Runtime Limit Guard Summary

```text
PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD
QCPU RUNTIME LIMIT GUARD READY
```

## Boundary Statement

The policy engine is software-only.

It does not mutate hardware.

Standard workload decision:

    ALLOW_STANDARD_WORKLOAD

Heavy workload decision:

    BLOCK_HEAVY_WORKLOAD

Final policy:

    ALLOW_STANDARD_WORKLOAD

Core status:

    PASS: CORE_NOT_MUTATED_BY_RUNTIME_POLICY_ENGINE

Safety:

    PASS: NON_DESTRUCTIVE_RUNTIME_POLICY_ENGINE

## Verdict

QCPU RUNTIME POLICY ENGINE READY
