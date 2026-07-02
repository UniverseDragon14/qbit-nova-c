# QCPU Hardware Reality Probe

QCPU Hardware Reality Probe is the first hardware-aware boundary layer for QBIT NOVA C.

It checks the real host hardware safely and reports what is physically present.

## Purpose

This layer proves:

- the host machine can be identified
- temperature and throttling can be checked safely
- physical QCPU hardware absence can be reported honestly
- virtual QCPU readiness can be confirmed
- no destructive hardware action is performed

## Key boundary

This project does not turn Raspberry Pi, phones, or classical CPUs into physical quantum hardware.

QBIT NOVA C creates a software-defined Virtual QCPU runtime.

## Run

    ./scripts/qcpu_hardware_probe.sh

## Output

    build/qcpu_hardware_probe.md

## Safety boundary

This probe is read-only.

It does not perform:

- voltage control
- overclocking
- kernel patching
- GPIO mutation
- hardware flashing
- destructive device access
