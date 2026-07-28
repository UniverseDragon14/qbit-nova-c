# NOVA Hypercube Runtime Snapshot

Generated UTC: 2026-07-20T10:30:36Z

## Git

Latest tag: v4.3

Latest commit:

    ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence

## Runtime Identity

```json
{
  "node": "QBIT_NOVA_VIRTUAL_QCPU",
  "mode": "virtual",
  "host": "nova-pi",
  "arch": "aarch64",
  "kernel": "6.18.34+rpt-rpi-2712",
  "cpu_model": "Raspberry Pi 5 Model B Rev 1.0",
  "boot_time_utc": "2026-07-20T10:30:36Z",
  "runtime": "QBIT_NOVA_C",
  "safety_boundary": "software virtual QCPU layer, not physical quantum hardware"
}
```

## Session

```text
QCPU_MODE=virtual
QCPU_RUNTIME=QBIT_NOVA_C
QCPU_HOSTNAME=nova-pi
QCPU_ARCH=aarch64
QCPU_KERNEL=6.18.34+rpt-rpi-2712
QCPU_CPU_MODEL=Raspberry Pi 5 Model B Rev 1.0
QCPU_BOOT_TIME_UTC=2026-07-20T10:30:36Z
QCPU_BOUNDARY=software_virtual_qcpu_not_physical_quantum_hardware
```

## Bell Proof Summary

```text
bad count: 0 BELL PROOF PASSED 
```

## QASM Preview

```qasm
OPENQASM 3.0;
include "stdgates.inc";

qubit[2] q;
bit[2] c;

// QBIT NOVA name map
// q -> q[0]
// p -> q[1]

h q[0];
cx q[0], q[1];
c[0] = measure q[0];
c[1] = measure q[1];
```

## Safety Boundary

This is a software virtual QCPU / NOVA Hypercube Runtime layer.
It does not claim that the device is physical quantum hardware.
