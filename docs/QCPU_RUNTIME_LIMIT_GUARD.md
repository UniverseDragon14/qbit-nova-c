# QCPU Runtime Limit Guard

QCPU Runtime Limit Guard turns the hardware capability map into safe runtime limits.

## Purpose

This layer proves:

- hardware capability can be read
- safe runtime mode can be selected
- thermal limits can be recorded
- memory and disk limits can be recorded
- the Virtual QCPU can avoid unsafe workload escalation
- no hardware mutation is performed

## Key boundary

This is a software guard.

It does not overclock, patch the kernel, change voltage, mutate GPIO, flash firmware, or access hardware destructively.

## Run

    ./scripts/qcpu_runtime_limit_guard.sh

## Outputs

    .qcpu/runtime_limits.env
    build/qcpu_runtime_limit_guard.md

## Modes

| Mode | Meaning |
|---|---|
| STANDARD_VIRTUAL_QCPU_MODE | Safe normal mode |
| CONSERVATIVE_VIRTUAL_QCPU_MODE | Use lighter workloads |
| REST_REQUIRED_MODE | Pause heavy work and cool down |

## Safety rule

If the host is hot, throttled, low on RAM, or low on disk, the guard recommends a safer mode.
