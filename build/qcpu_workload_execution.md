# QCPU Workload Execution Wrapper

Generated UTC: 2026-07-20T10:30:43Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Admission Read

| Field | Value |
|---|---|
| QCPU mode | STANDARD_VIRTUAL_QCPU_MODE |
| Final policy | ALLOW_STANDARD_WORKLOAD |
| Admission status | PASS: WORKLOAD_ADMISSION_POLICY_ENFORCED |
| Standard workload | standard_bell_proof |
| Standard admission | ADMIT_WORKLOAD |
| Heavy workload | heavy_1000_shot_run |
| Heavy admission | REJECT_WORKLOAD |

## Execution Decisions

| Workload | Admission | Execution | Status |
|---|---|---|---|
| standard_bell_proof | ADMIT_WORKLOAD | EXECUTED | PASS: STANDARD_WORKLOAD_EXECUTED |
| heavy_1000_shot_run | REJECT_WORKLOAD | BLOCKED | PASS: HEAVY_WORKLOAD_NOT_EXECUTED |

## Execution Env

```text
QCPU_EXECUTION_MODE=STANDARD_VIRTUAL_QCPU_MODE
QCPU_FINAL_POLICY=ALLOW_STANDARD_WORKLOAD
QCPU_STANDARD_WORKLOAD=standard_bell_proof
QCPU_STANDARD_ADMISSION=ADMIT_WORKLOAD
QCPU_STANDARD_EXECUTION=EXECUTED
QCPU_STANDARD_EXECUTION_STATUS=PASS: STANDARD_WORKLOAD_EXECUTED
QCPU_HEAVY_WORKLOAD=heavy_1000_shot_run
QCPU_HEAVY_ADMISSION=REJECT_WORKLOAD
QCPU_HEAVY_EXECUTION=BLOCKED
QCPU_HEAVY_EXECUTION_STATUS=PASS: HEAVY_WORKLOAD_NOT_EXECUTED
QCPU_EXECUTION_WRAPPER_STATUS=PASS: WORKLOAD_EXECUTION_POLICY_ENFORCED
QCPU_EXECUTION_CREATED_UTC=2026-07-20T10:30:43Z
```

## Standard Workload Proof Summary

```text
run 1: |11> OK
run 2: |00> OK
run 3: |00> OK
run 4: |11> OK
run 5: |11> OK
run 6: |00> OK
run 7: |11> OK
run 8: |00> OK
run 9: |00> OK
run 10: |00> OK
bad count: 0
BELL PROOF PASSED
Only |00> and |11> appeared.
```

## Heavy Workload Block Log

```text
heavy workload rejected by admission controller
workload: heavy_1000_shot_run
decision: REJECT_WORKLOAD
execution: BLOCKED
```

## Boundary Statement

The execution wrapper is software-only.

It executes admitted workloads only.

Standard workload:

    PASS: STANDARD_WORKLOAD_EXECUTED

Heavy workload:

    PASS: HEAVY_WORKLOAD_NOT_EXECUTED

Wrapper status:

    PASS: WORKLOAD_EXECUTION_POLICY_ENFORCED

Core status:

    PASS: CORE_NOT_MUTATED_BY_WORKLOAD_EXECUTION

Safety:

    PASS: NON_DESTRUCTIVE_WORKLOAD_EXECUTION_WRAPPER

## Verdict

QCPU WORKLOAD EXECUTION WRAPPER READY
