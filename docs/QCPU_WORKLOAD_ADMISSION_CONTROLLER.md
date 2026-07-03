# QCPU Workload Admission Controller

QCPU Workload Admission Controller reads the runtime policy engine output and admits or rejects workload requests.

## Purpose

This layer proves:

- runtime policy can be read
- workload requests can be evaluated
- standard safe workloads can be admitted
- heavy workloads can be rejected
- the admission decision can be recorded
- the core engine remains unchanged
- no destructive hardware action is performed

## Rule

The admission controller is software-only.

It does not overclock, patch the kernel, change voltage, mutate GPIO, flash firmware, or access hardware destructively.

## Run

    ./scripts/qcpu_workload_admission.sh

## Outputs

    .qcpu/workload_admission.env
    build/qcpu_workload_admission.md

## Decision examples

| Workload | Expected Decision |
|---|---|
| standard_bell_proof | ADMIT_WORKLOAD |
| heavy_1000_shot_run | REJECT_WORKLOAD |

## Meaning

QBIT NOVA can now say:

    I know the policy.
    I inspect the workload.
    I admit safe work.
    I reject heavy work.
    I explain the decision.
