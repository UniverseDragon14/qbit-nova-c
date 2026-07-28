# QNOVA Public Demo CLI

Generated UTC: 2026-07-20T10:30:46Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Latest tag | v4.3 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Public CLI Summary

| Check | Result |
|---|---|
| Public demo runtime | PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY |
| Demo status | PASS: QCPU_PUBLIC_DEMO_STATUS_CONFIRMED |
| Bell proof | PASS: PUBLIC_BELL_PROOF_READY |
| OpenQASM export | PASS: PUBLIC_QASM_EXPORT_READY |
| Public release manifest | PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO |
| Safety | PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME |
| Boundary | PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY |

## CLI Decision

| Field | Value |
|---|---|
| Decision | ALLOW_PUBLIC_CLI |
| Status | PASS: QNOVA_PUBLIC_DEMO_CLI_READY |

## Human Demo Command

    ./scripts/qnova_demo.sh

## Public Explanation

QBIT NOVA C is a software virtual QCPU runtime and proof chain.

It runs on a classical host.

It does not mutate hardware.

It does not claim physical quantum hardware.

## Demo Receipt

Virtual QCPU:

    PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY

Bell proof:

    PASS: PUBLIC_BELL_PROOF_READY

OpenQASM export:

    PASS: PUBLIC_QASM_EXPORT_READY

Release manifest:

    PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY

Safety:

    PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME

Boundary:

    PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY

## Verdict

QNOVA PUBLIC DEMO CLI READY

PASS: QNOVA_PUBLIC_DEMO_CLI_READY
