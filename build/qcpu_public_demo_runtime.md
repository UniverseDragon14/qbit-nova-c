# QCPU Public Demo Runtime

Generated UTC: 2026-07-20T10:30:46Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Latest tag | v4.3 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Public Demo Chain

| Check | Result |
|---|---|
| Public release manifest | PASS: PUBLIC_RELEASE_MANIFEST_READY |
| Public manifest status | PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY |
| Virtual QCPU boot | PASS: VIRTUAL_QCPU_BOOT_READY |
| Bell proof | PASS: PUBLIC_BELL_PROOF_READY |
| QASM export | PASS: PUBLIC_QASM_EXPORT_READY |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO |
| Safety | PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME |
| Boundary | PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY |

## Demo Decision

| Field | Value |
|---|---|
| Decision | ALLOW_PUBLIC_DEMO |
| Status | PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY |

## Public Demo Receipt

Mode:

    STANDARD_VIRTUAL_QCPU_MODE

Virtual QCPU:

    PASS: VIRTUAL_QCPU_BOOT_READY

Bell proof:

    PASS: PUBLIC_BELL_PROOF_READY

OpenQASM export:

    PASS: PUBLIC_QASM_EXPORT_READY

Release manifest:

    PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY

Core status:

    PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO

Safety:

    PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME

## Boundary Statement

This public demo runtime is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

It proves that QBIT NOVA C can run a safe virtual QCPU public demo chain on a classical host.

## Verdict

QCPU PUBLIC DEMO RUNTIME READY

PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY
