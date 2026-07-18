# Novakutty - OpenAI Build Week Judge Guide

Creator and owner: **Universal Dragon Aslam**

Core technology: **QBIT NOVA C**

User-facing assistant identity: **NOVA / EVE**

Novakutty and QBIT NOVA C are conceived, directed, and owned by Universal
Dragon Aslam. Development tools are credited only in the event-required
disclosure below; they are not the product brand, creator, or owner.

This creator identity predates the Build Week changes. It is recorded in the
[Universal Dragon Aslam Receipt](docs/UNIVERSAL_DRAGON_ASLAM_RECEIPT.md), committed
at the v4.1.2 CI-recovery checkpoint.

NOVA / EVE is the product-facing assistant identity. The current Stage 2B
deliverable proves the approval and Virtual QCPU runtime path; it does not claim
that a separate EVE model-inference backend already exists.

## NOVA / EVE assistant identity

The planned product roles are:

- **NOVA** presents approval decisions, runtime state, and reproducible proof
  evidence.
- **EVE** provides the conversational explanation layer and safe next-step
  guidance.
- Both remain subordinate to the approval gate and the software-only truth
  boundary.

These are Novakutty product identities owned by Universal Dragon Aslam. A
future inference provider can sit behind the interface, but its vendor name
does not replace the NOVA / EVE user-facing identity. Any required competition
or dependency disclosure remains factual and separate from product branding.

## Category

Developer Tools

## One-line pitch

Novakutty is an approval-first developer tool, built on QBIT NOVA C, that
admits safe workloads, rejects unsafe ones, and executes verified
quantum-style workloads through a software Virtual QCPU with explicit evidence
and honest hardware boundaries.

## Problem

Low-level and AI-assisted compute workflows can cross safety boundaries without
leaving a clear explanation of what was approved, executed, rejected, or
verified. Quantum-style demos also frequently blur the distinction between a
simulator and physical quantum hardware.

## Solution

Novakutty combines Universal Dragon Aslam's QBIT NOVA C core with:

- an approval gate driven by host limits and runtime policy;
- a C language, bytecode VM, state-vector engine, and OpenQASM bridge;
- a software Virtual QCPU protocol with bounded I/O;
- deterministic timeout, cancellation, recovery, and exclusivity proofs;
- a v4.7 userspace frontend connected to the verified v4.6 `qcpud` backend;
  and
- evidence receipts that state exactly what passed and what was rejected.

It does not claim that a Raspberry Pi, phone, or classical CPU becomes physical
quantum hardware.

## Judge quickstart

Supported platforms:

- Linux on x86_64 or AArch64;
- GCC with C11 and pthread support;
- Bash, Python 3, and standard Linux command-line utilities.

Run:

```bash
git clone https://github.com/UniverseDragon14/qbit-nova-c.git
cd qbit-nova-c
bash scripts/qnova_build_week_demo.sh
```

Expected final marker:

```text
PASS: NOVAKUTTY_BUILD_WEEK_DEMO_READY
```

For the full regression suite:

```bash
bash scripts/test_all.sh
```

Expected final marker:

```text
ALL QBIT NOVA TESTS PASSED
```

The pull-request workflow also runs whitespace checks, Bash syntax,
ShellCheck, GCC's static analyzer, the full regression suite, and this exact
judge demo before the branch can be treated as verified.

## Ownership and required development-tool disclosure

Universal Dragon Aslam remains the creator, project director, and owner.
QBIT NOVA C existed before this Build Week work. For transparent compliance
with the event requirements, Codex assisted Universal Dragon Aslam by:

- inspecting the existing UAPI, threat model, qcpud transport, tests, and proof
  scripts before changing code;
- designing the Stage 2B adapter inside the existing truth and safety boundary;
- implementing explicit request/response translation and deadline-aware I/O;
- creating focused STATUS, GHZ, missing-backend, stall, and cleanup tests;
- building the one-command judge demo; and
- reviewing the result against the Build Week submission and judging criteria.

Codex is a development assistant, not the product author or owner. The
implementation remains understandable C with deterministic tests; assistant
output is not treated as evidence unless the compiled proof reproduces it.

## GPT-5.6 evidence to finalize before submission

The build workspace included a supplied OpenAI GPT-5.6 System Card as
background material. It is not committed to this repository, and a document is
not proof that a model built the project. Before submission, the owner must:

1. confirm that the primary Codex build session used GPT-5.6 for a meaningful
   portion of the implementation or review;
2. describe that exact portion here in the owner's own words;
3. retrieve the `/feedback` Session ID from that primary build session; and
4. include the same accurate explanation in the demo voiceover.

Do not replace these steps with an unsupported model claim.

## Submission items for Universal Dragon Aslam

- Choose and add an open-source license.
- Use the Novakutty brand consistently in the gallery, video, and submission.
- Record and publish the under-three-minute YouTube demo with voiceover.
- Add the verified `/feedback` Session ID.
- Write the final Devpost project description in Universal Dragon Aslam's own
  voice.
- Submit the final project before the deadline.

Copy-ready form answers and an under-three-minute voiceover are available in
[the Devpost submission draft](DEVPOST_SUBMISSION_DRAFT.md).
