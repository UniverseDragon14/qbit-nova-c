# QCPU Public Release Manifest

Generated UTC: 2026-07-20T10:30:46Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |
| Latest tag | v4.3 |
| v3 tag count | 13 |

## Release Manifest Checks

| Check | Result |
|---|---|
| Release seal | PASS: RELEASE_SEAL_VERIFIED |
| Release ready status | PASS: RELEASE_READY_STATUS_PRESENT |
| Release tag | PASS: RELEASE_TAG_PRESENT |
| v3 release chain | PASS: V3_RELEASE_CHAIN_PRESENT |
| Required files | PASS: RELEASE_FILES_PRESENT |
| CI-safe backup snapshot | PASS: RELEASE_BACKUP_PRESENT |
| Core status | PASS: CORE_NOT_MUTATED_BY_PUBLIC_MANIFEST |
| Safety | PASS: NON_DESTRUCTIVE_PUBLIC_MANIFEST |

## Release Decision

| Field | Value |
|---|---|
| Decision | ALLOW_PUBLIC_RELEASE_MANIFEST |
| Status | PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY |

## v3 Tags

v3.0 v3.1 v3.2 v3.3 v3.4 v3.5 v3.5.1 v3.6 v3.7 v3.8 v3.9 v3.9.1 v3.9.2

## Required File Check


README.md
scripts/test_all.sh
scripts/qcpu_release_readiness_seal.sh
docs/QCPU_RELEASE_READINESS_SEAL.md
scripts/qcpu_ci_evidence_gate.sh
docs/QCPU_CI_EVIDENCE_GATE.md
scripts/qcpu_ci_evidence.sh
docs/QCPU_CI_EVIDENCE_REPORTER.md
scripts/qcpu_workload_execute.sh
docs/QCPU_WORKLOAD_EXECUTION_WRAPPER.md


## Release Seal Summary

PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
QCPU_CI_GATE_SAFETY=PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
| Safety | PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE | Software-only non-destructive gate |
QCPU_CI_GATE_SAFETY=PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
    PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
build/qcpu_ci_evidence_gate.md:| Safety | PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE | Software-only non-destructive gate |
build/qcpu_ci_evidence_gate.md:QCPU_CI_GATE_SAFETY=PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
build/qcpu_ci_evidence_gate.md:    PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
.qcpu/ci_evidence_gate.env:QCPU_CI_GATE_SAFETY=PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
release decision: ALLOW_RELEASE_SEAL
QCPU_RELEASE_DECISION=ALLOW_RELEASE_SEAL
QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY
QCPU_SAFETY_STATUS=PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE
| Safety boundary | PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE |
| Decision | ALLOW_RELEASE_SEAL |
QCPU RELEASE READINESS SEAL READY
QCPU RELEASE READINESS SEAL READY
build/qcpu_release_readiness_seal.md:| Decision | ALLOW_RELEASE_SEAL |
build/qcpu_release_readiness_seal.md:QCPU RELEASE READINESS SEAL READY
.qcpu/release_readiness_seal.env:QCPU_RELEASE_DECISION=ALLOW_RELEASE_SEAL
.qcpu/release_readiness_seal.env:QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY

## Boundary Statement

This public release manifest is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

It states clearly that QBIT NOVA C is a software virtual QCPU runtime and proof chain.

Public release is allowed only when release seal, CI evidence, virtual QCPU support, safety boundary, tag chain, and required files are present.

## Verdict

QCPU PUBLIC RELEASE MANIFEST READY

PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY
