# QCPU Hardware Capability Map

QCPU Hardware Capability Map reads the real host capability and recommends a safe Virtual QCPU runtime mode.

## Purpose

This layer proves:

- CPU architecture can be mapped
- CPU core count can be mapped
- RAM capacity can be mapped
- disk capacity can be mapped
- thermal/throttle state can be mapped
- available runtime tools can be checked
- a safe Virtual QCPU mode can be recommended

## Key boundary

This is not physical quantum hardware conversion.

This is a read-only capability map for safely running a software-defined Virtual QCPU runtime.

## Run

    ./scripts/qcpu_hardware_capability_map.sh

## Output

    build/qcpu_hardware_capability_map.md

## Safety boundary

This map is read-only.

It does not perform:

- voltage control
- overclocking
- GPIO mutation
- kernel patching
- firmware flashing
- destructive device access

## Recommended modes

| Mode | Meaning |
|---|---|
| STANDARD_VIRTUAL_QCPU_MODE | Hardware is cool, not throttled, enough RAM/disk |
| CONSERVATIVE_VIRTUAL_QCPU_MODE | Hardware is usable but should avoid heavy workloads |
| REST_REQUIRED_MODE | Temperature/throttling suggests rest before heavy work |
