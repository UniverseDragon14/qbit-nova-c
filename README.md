# QBIT NOVA C

[![QBIT NOVA CI](https://github.com/UniverseDragon14/qbit-nova-c/actions/workflows/qbit-nova-ci.yml/badge.svg)](https://github.com/UniverseDragon14/qbit-nova-c/actions/workflows/qbit-nova-ci.yml)


Creator: Universal Dragon Aslam
Project: QBIT NOVA C

QBIT NOVA C is a small C-based quantum-style language, bytecode VM, state-vector simulator, and virtual QMSG/QCPU layer.

Important safety note:
QBIT NOVA C does not claim that a normal phone or Raspberry Pi processor becomes a physical quantum chip.
It builds a software virtual quantum processor layer on top of classical hardware.

## Current Capabilities

- .qn language syntax
- Lexer, parser, AST, compiler
- Bytecode VM
- Safe action adapter contract
- Qbit simulation
- Hadamard gate
- CNOT simulation
- Entangle simulation
- Repeat blocks
- 2-qbit state-vector core
- Unified qnova runner
- QMSG virtual packet layer
- QMSG virtual qbit register view
- OpenQASM exporter

## Build

Run:

    gcc src/qnova.c \
        src/lexer/lexer.c \
        src/parser/parser.c \
        src/compiler/compiler.c \
        src/vm/byte_vm.c \
        src/vm/state2_vm.c \
        src/quantum/state2.c \
        src/adapter/safe_adapter.c \
        -o qnova -lm

## Run Examples

    ./qnova examples/full_test.qn
    ./qnova examples/bell_state2.qn
    ./qnova examples/qmsg_hi.qn
    ./qnova examples/qmsg_register.qn

## QMSG Register Proof

Source idea:

    qmsg "HI"
    encode qmsg
    say qmsg
    measure qmsg
    decode qmsg

Expected output:

    [QMSG] bits: 0100100001001001
    [QMSG] virtual qbit register:
    q0=|0> q1=|1> q2=|0> q3=|0> q4=|1> q5=|0> q6=|0> q7=|0>
    q8=|0> q9=|1> q10=|0> q11=|0> q12=|1> q13=|0> q14=|0> q15=|1>
    [QMSG] decoded message: HI

## Version Timeline

| Version | Feature |
|---|---|
| v1.0 | 2-qbit state-vector core |
| v1.1 | .qn to state-vector pipeline |
| v1.2 | Unified qnova runner |
| v1.3 | QMSG virtual packet layer |
| v1.4 | QMSG virtual qbit register view |
| v1.5 | Public proof pack |
| v1.6 | GitHub CI proof tests |
| v1.7 | CI status badge |
| v1.8 | OpenQASM export |

## Vision

QBIT NOVA is a seven-layer experiment:

1. Classical processor
2. QBIT packet layer
3. Virtual QPU
4. State-vector brain
5. QMSG message layer
6. Pi5 quantum node
7. Future real QPU bridge through OpenQASM


## OpenQASM Export

Build exporter:

    gcc src/tools/qasm_export.c \
        src/lexer/lexer.c \
        src/parser/parser.c \
        src/compiler/compiler.c \
        -o qnova-qasm -lm

Run:

    ./qnova-qasm examples/bell_qasm.qn

Expected output includes:

    OPENQASM 3.0;
    include "stdgates.inc";
    qubit[2] q;
    bit[2] c;
    h q[0];
    cx q[0], q[1];
    c[0] = measure q[0];
    c[1] = measure q[1];


## Bell State Proof

Run:

    ./scripts/proof_bell.sh 20

Expected result:

    Only |00> and |11> appeared.

This proves the QBIT NOVA state-vector engine preserves Bell-state measurement correlation.


## v2.0 Release Checkpoint

QBIT NOVA C v2.0 is the first public release checkpoint.

It includes:

- Public release notes
- Changelog
- v2 architecture documentation
- Quick demo documentation
- Demo script
- Bell-state proof
- OpenQASM export
- QMSG virtual qbit register
- CI proof tests

Run:

    ./scripts/demo.sh
    ./scripts/test_all.sh

v2.0 keeps the safety boundary clear: this is a software virtual quantum processor layer, not physical quantum hardware conversion.
\n| v2.0 | Public release checkpoint |\n| v2.1 | QASM file export bridge |\n| v2.2 | Virtual QCPU boot layer |\n| v2.3 | NOVA Hypercube Runtime status layer |\n| v2.4 | NOVA Hypercube snapshot report |\n

## QASM File Export

Run:

    ./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm

This creates:

    build/bell.qasm

The exported file can be used later as the bridge toward Qiskit or real quantum cloud workflows.


## Virtual QCPU Boot

Run:

    ./scripts/qcpu_boot.sh

This creates a local virtual QCPU session:

    .qcpu/session.env
    build/qcpu_node.json
    build/bell.qasm

This does not convert local hardware into physical quantum hardware.
It creates a QBIT NOVA software runtime identity layer on top of classical hardware.


## NOVA Hypercube Runtime

Run:

    ./scripts/hypercube_status.sh

This verifies the local NOVA Hypercube Runtime identity using the existing virtual QCPU boot layer.

It checks:

- QCPU node identity
- QCPU session file
- QASM export file
- software safety boundary

This is a software-defined quantum-style runtime layer, not physical quantum hardware.


## NOVA Hypercube Snapshot

Run:

    ./scripts/hypercube_snapshot.sh

This creates:

    build/hypercube_snapshot.md

The snapshot records QCPU node identity, Bell proof summary, QASM preview, Git tag, and safety boundary.


## QCPU Boundary Fit Matrix

Run:

    ./scripts/qcpu_boundary_fit.sh

This creates:

    build/qcpu_boundary_fit.md

The matrix records which stones fit and which stones fall.

Expected result on normal Raspberry Pi/mobile hardware:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND
    PASS: VIRTUAL_QCPU_READY

This is honest boundary testing, not fake physical quantum hardware claiming.


## QCPU Noise Injection Matrix

Run:

    ./scripts/qcpu_noise_injection.sh

This creates:

    build/qcpu_noise_injection.md

The test confirms that clean Bell proof passes while a synthetic noisy Bell sample is detected honestly.

Expected:

    PASS: CLEAN_BELL_PROOF_READY
    EXPECTED_DETECT: NOISY_BELL_OUTPUT_FOUND
    QCPU NOISE INJECTION MATRIX READY


## QCPU Recovery Matrix

Run:

    ./scripts/qcpu_recovery_matrix.sh

This creates:

    build/qcpu_recovery_matrix.md

The test detects synthetic noise, enters recovery mode, re-runs clean Bell proof, reboots the virtual QCPU, and confirms that the core engine was not mutated.

Expected:

    PASS: NOISE_DETECTED
    PASS: RECOVERY_MODE_ACTIVE
    PASS: CLEAN_BELL_PROOF_AFTER_RECOVERY
    PASS: VIRTUAL_QCPU_REBOOTED
    QCPU RECOVERY MATRIX READY


## QCPU Fault Memory

Run:

    ./scripts/qcpu_fault_memory.sh

This creates:

    logs/qcpu_fault_memory.log
    build/qcpu_fault_memory.md

The test records a local proof trail after noise detection and recovery.

Expected:

    QCPU FAULT MEMORY READY
    PASS: CORE_NOT_MUTATED_BY_FAULT_MEMORY


## QCPU Fault Timeline

Run:

    ./scripts/qcpu_fault_timeline.sh

This creates:

    build/qcpu_fault_timeline.md

The timeline summarizes local fault memory events into a readable proof history.

Expected:

    QCPU FAULT TIMELINE READY
    PASS: CORE_NOT_MUTATED_BY_FAULT_TIMELINE

| v3.0 | QCPU Hardware Reality Probe |
| v3.1 | QCPU Hardware Capability Map |


## QCPU Hardware Reality Probe

Run:

    ./scripts/qcpu_hardware_probe.sh

This creates:

    build/qcpu_hardware_probe.md

The probe checks real host hardware safely and reports the honest physical boundary.

Expected:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND
    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST
    QCPU HARDWARE REALITY PROBE READY


## QCPU Hardware Capability Map

Run:

    ./scripts/qcpu_hardware_capability_map.sh

This creates:

    build/qcpu_hardware_capability_map.md

The map reads host capability safely and recommends a Virtual QCPU runtime mode.

Expected:

    QCPU HARDWARE CAPABILITY MAP READY
    PASS: READ_ONLY_CAPABILITY_MAP


## QCPU Runtime Limit Guard

Run:

    ./scripts/qcpu_runtime_limit_guard.sh

This creates:

    .qcpu/runtime_limits.env
    build/qcpu_runtime_limit_guard.md

The guard converts hardware capability into safe software runtime limits.

Expected:

    QCPU RUNTIME LIMIT GUARD READY
    PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD


## QCPU Runtime Policy Engine

Run:

    ./scripts/qcpu_runtime_policy_engine.sh

This creates:

    .qcpu/runtime_policy.env
    build/qcpu_runtime_policy_engine.md

The policy engine reads runtime limits and decides whether workloads are allowed or blocked.

Expected:

    ALLOW_STANDARD_WORKLOAD
    BLOCK_HEAVY_WORKLOAD
    QCPU RUNTIME POLICY ENGINE READY


## QCPU Workload Admission Controller

Run:

    ./scripts/qcpu_workload_admission.sh

This creates:

    .qcpu/workload_admission.env
    build/qcpu_workload_admission.md

The admission controller reads runtime policy and admits or rejects workload requests.

Expected:

    ADMIT_WORKLOAD
    REJECT_WORKLOAD
    QCPU WORKLOAD ADMISSION CONTROLLER READY


## QCPU Workload Execution Wrapper

Run:

    ./scripts/qcpu_workload_execute.sh

This creates:

    .qcpu/workload_execution.env
    build/qcpu_workload_execution.md

The execution wrapper executes admitted workloads and blocks rejected workloads.

Expected:

    PASS: STANDARD_WORKLOAD_EXECUTED
    PASS: HEAVY_WORKLOAD_NOT_EXECUTED
    QCPU WORKLOAD EXECUTION WRAPPER READY

## QCPU CI Evidence Reporter

v3.6 adds a CI evidence reporter.

It records CI-safe proof that QBIT NOVA can run with hardware fallback protection.

```text
QCPU CI EVIDENCE REPORTER READY
```

## QCPU CI Evidence Gate

v3.7 adds a CI evidence gate.

It checks the v3.6 CI Evidence Reporter output and opens only when CI-safe fallback, virtual QCPU support, honest physical QCPU expected-fail, core safety, and non-destructive boundary markers are present.

```text
QCPU CI EVIDENCE GATE READY
PASS: CI_EVIDENCE_GATE_OPENED
```

## QCPU Release Readiness Seal

v3.8 adds a release readiness seal.

It checks that the CI Evidence Gate is open, the virtual QCPU path is supported, the physical QCPU boundary remains honest, and the release chain is safe before allowing the release seal.

Expected output:

QCPU RELEASE READINESS SEAL READY
QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY
