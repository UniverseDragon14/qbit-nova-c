# QCPU Workload Execution Wrapper

QCPU Workload Execution Wrapper executes only admitted QCPU workloads and blocks rejected workloads.

## Purpose

This layer proves:

- workload admission can be read
- admitted workloads can execute
- rejected workloads are not executed
- execution results can be recorded
- the core engine remains unchanged
- no destructive hardware action is performed

## Rule

The execution wrapper is software-only.

It does not overclock, patch the kernel, change voltage, mutate GPIO, flash firmware, or access hardware destructively.

## Run

    ./scripts/qcpu_workload_execute.sh

## Outputs

    .qcpu/workload_execution.env
    build/qcpu_workload_execution.md
    logs/qcpu_execution_standard_bell.log
    logs/qcpu_execution_heavy_blocked.log

## Meaning

QBIT NOVA can now say:

    I read admission.
    I execute admitted work.
    I refuse rejected work.
    I record what happened.
