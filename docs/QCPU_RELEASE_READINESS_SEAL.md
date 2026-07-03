# QCPU Release Readiness Seal

The QCPU Release Readiness Seal is the v3.8 release checkpoint.

It verifies that the CI Evidence Gate is open before marking the runtime as release-ready.

## Checks

- CI evidence gate is ready
- CI evidence gate opened
- honest physical QCPU expected-fail is present
- virtual QCPU support is present
- non-destructive safety boundary is present
- release chain files are present
- v3 tags are present
- core engine is not mutated

## Boundary

This is software-only.

It does not mutate hardware.

It does not claim that Raspberry Pi or any classical device became physical quantum hardware.

## Expected verdict

QCPU RELEASE READINESS SEAL READY
QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY
