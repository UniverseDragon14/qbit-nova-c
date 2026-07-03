# QCPU Runtime Policy Engine

QCPU Runtime Policy Engine reads the runtime limit guard output and makes safe workload decisions.

## Purpose

This layer proves:

- runtime limits can be read
- standard workloads can be allowed
- heavy workloads can be blocked
- policy decisions can be written to a local report
- the core engine remains unchanged
- no destructive hardware action is performed

## Rule

The policy engine is software-only.

It does not overclock, patch the kernel, change voltage, mutate GPIO, flash firmware, or access hardware destructively.

## Run

    ./scripts/qcpu_runtime_policy_engine.sh

## Outputs

    .qcpu/runtime_policy.env
    build/qcpu_runtime_policy_engine.md

## Decision examples

| Workload | Expected Decision |
|---|---|
| Standard workload | ALLOW_STANDARD_WORKLOAD |
| Heavy workload | BLOCK_HEAVY_WORKLOAD |

## Meaning

QBIT NOVA can now say:

    I know my runtime limits.
    I can allow safe work.
    I can block heavy work.
    I can explain the decision.
