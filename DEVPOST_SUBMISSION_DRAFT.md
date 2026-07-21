# Novakutty - Devpost Submission Draft

Creator and owner: **Universal Dragon Aslam**

Status: repository evidence, MIT license, public demo video, pull request,
and Codex/GPT-5.6 session evidence are ready. Pending Devpost form completion
and final submission.

## Form answers

- **Project name:** Novakutty
- **Tagline:** Approval-first Virtual QCPU runtime with reproducible safety proofs
- **Submitter type:** Individual
- **Country of residence:** United Arab Emirates
- **Category:** Developer Tools
- **Code repository:** https://github.com/UniverseDragon14/qbit-nova-c
- **Built with:** C, Bash, Python, GitHub Actions, Unix domain sockets,
  OpenQASM, Codex, GPT-5.6
- **Public demo video:** `https://youtu.be/c_Xe6yny3w4`
- **Primary Codex /feedback Session ID:** `019f83e0-f277-7540-ba39-e0583bde8233`
- **GPT-5.6 Sol verification Session ID:** `019f8513-79ca-7e81-9063-9f69aa657945`
- **Verified pull request / CI:** `https://github.com/UniverseDragon14/qbit-nova-c/pull/39`
- **GPT-5.6 Sol verification receipt:**
  `docs/OPENAI_BUILD_WEEK_GPT56_SOL_VERIFICATION_RECEIPT.md`

## Project description

### Inspiration

Low-level and AI-assisted compute workflows can execute complex actions without
leaving a clear, reproducible answer to four basic questions: what was
requested, what was approved, what actually ran, and what was rejected.
Quantum-style demos also frequently blur the boundary between a simulator and
physical quantum hardware.

I created Novakutty to make that boundary explicit. It is built on my existing
QBIT NOVA C ecosystem and turns approval decisions, bounded execution, failure
recovery, and proof evidence into one developer workflow.

### What it does

Novakutty is an approval-first developer tool for a software Virtual QCPU. It:

- admits a bounded standard workload and rejects an unsafe heavy workload;
- runs a C language, bytecode VM, state-vector engine, and OpenQASM bridge;
- exposes a stable userspace QCPU contract without creating a kernel module or
  device node;
- translates STATUS and GHZ requests through a bounded Unix-socket adapter to
  the verified `qcpud` backend;
- handles missing backends, stalls, cancellation, disconnects, and restarts;
  and
- produces deterministic PASS/FAIL markers and a judge-readable evidence
  receipt.

The truth boundary is intentional: this is a software Virtual QCPU running on
a classical host. It does not claim that a Raspberry Pi, phone, or normal CPU
becomes physical quantum hardware.

### How I built it

The core is written in understandable C11. The Stage 2B data path connects the
v4.7 pure-userspace frontend to the existing v4.6 `qcpud` Unix-socket daemon.
Requests and responses use fixed-size, explicit little-endian frames. One
absolute monotonic deadline is shared across connect, write, and read so each
transport step cannot restart the timeout budget.

The proof suite compiles with warnings as errors and exercises the live STATUS
and GHZ bridge, Q32.32 norm conversion, offline mapping, bounded stalls,
disconnect wake-up, `SIGPIPE` suppression, graceful cleanup, and restart
recovery. GitHub Actions additionally runs whitespace checks, Bash syntax,
ShellCheck, GCC's static analyzer, the full regression suite, and the exact
one-command judge demo.

### How Codex and GPT-5.6 were used

Universal Dragon Aslam conceived, directed, and owns Novakutty and QBIT NOVA
C. Codex with GPT-5.6 accelerated the Build Week implementation by inspecting
the existing UAPI, threat model, transport, and test contracts; implementing
and reviewing the bounded Stage 2B adapter; finding and fixing an early-peer
`SIGPIPE` termination risk; adding focused regression tests and CI gates; and
turning the result into a one-command judge demo.

AI output was not accepted as proof by itself. Every claim is tied to compiled
code, deterministic tests, or GitHub CI evidence, and the final product and
engineering decisions remain Universal Dragon Aslam's.

On 2026-07-18, I completed a dedicated final Stage 2B verification and Devpost
evidence-review session that I identify as GPT-5.6 Sol. It independently reran
the one-command judge demo, the full regression suite, the focused qcpud
adapter proof, and the available local static gates against commit `da1dbb9`.
The exact results and limitations are recorded in
`docs/OPENAI_BUILD_WEEK_GPT56_SOL_VERIFICATION_RECEIPT.md`. This session
supports the final verification and review claim; it does not independently
establish which model performed the earlier implementation work.

### Challenges

The hardest part was connecting two already-stable contracts without weakening
either one. Timeout, cancellation, and file-descriptor reuse races had to stay
bounded, while backend failures needed to map cleanly into the public UAPI.
The hosted development sandbox can block required Unix-socket operations, so
the same proof is compiled locally and executed live by GitHub Actions on an
Ubuntu runner.

### Accomplishments

- Preserved the existing software-only truth boundary.
- Added a bounded qcpud adapter without kernel, root, device-node, TCP, or UDP
  actions.
- Added deterministic success, offline, timeout, disconnect, and restart
  coverage.
- Hardened socket writes against process-terminating `SIGPIPE`.
- Produced a single judge command and evidence receipt.
- Kept the implementation reproducible on x86_64 and AArch64 Linux.

### What I learned

Reliable agentic and low-level systems need independent evidence, not confident
status text. A single absolute deadline, explicit protocol translation, honest
hardware claims, and CI-backed receipts make the system easier to trust and
easier for another developer to reproduce.

### What's next

Next I will add the NOVA presentation layer and EVE explanation interface over
the same approval and proof boundary, expand the request vocabulary beyond GHZ,
and package the runtime for easier installation while keeping physical-hardware
claims and privileged actions explicitly outside the current scope.

## Private judge instructions field

No credentials are required.

```text
Supported platform: Ubuntu or another Linux distribution on x86_64/AArch64.
Requirements: GCC with C11/pthreads, Bash, Python 3, and standard Linux tools.

git clone https://github.com/UniverseDragon14/qbit-nova-c.git
cd qbit-nova-c
bash scripts/qnova_build_week_demo.sh

Expected final marker:
PASS: NOVAKUTTY_BUILD_WEEK_DEMO_READY

The demo performs no root, kernel-module, device-node, TCP/UDP, GPIO, or
physical-QPU action. GitHub Actions is the canonical live Unix-socket result.
```

## Under-three-minute demo voiceover

### 0:00-0:20 - Problem and identity

"I am Universal Dragon Aslam. I built Novakutty on my QBIT NOVA C ecosystem to
make low-level compute approval, execution, rejection, and proof visible. This
is a software Virtual QCPU on a classical host, not physical quantum hardware."

### 0:20-0:45 - Show the repository

Show `README.md`, `OPENAI_BUILD_WEEK.md`, and the Stage 2B architecture. Point
out the userspace-only boundary and the one-command demo.

### 0:45-1:50 - Run the demo

Run:

```bash
bash scripts/qnova_build_week_demo.sh
```

Show the standard workload being approved, the heavy workload being rejected,
and the live STATUS/GHZ, timeout, disconnect, and restart PASS markers.

### 1:50-2:20 - Show CI and evidence

Show the green GitHub pull-request checks and the generated receipt. Explain
that compiled tests and CI evidence, not assistant text, are the source of
truth.

### 2:20-2:50 - Codex and GPT-5.6 disclosure

"I conceived, directed, and own the project. Codex with GPT-5.6 accelerated the
Stage 2B architecture review, C implementation, bug finding, focused tests, CI
gates, and documentation. I reviewed the changes, and every claim is backed by
reproducible code and tests."

### 2:50-2:58 - Close

"Novakutty is an approval-first foundation for the Universal Dragon Aslam
ecosystem: honest boundaries, bounded execution, and proof before trust."

## Final submission checklist

- [x] GPT-5.6 Sol verification session is recorded with reproducible results.
- [x] MIT License is included on the repository `main` branch.
- [x] PR #39 is green and the verified PR/CI URL is inserted above.
- [x] PR #39 is merged and the judge command passes on the `main` branch.
- [x] Under-three-minute voiceover video is public on YouTube.
- [ ] `/feedback` Session ID is added to Devpost.
- [ ] Devpost description and private judge instructions are copied from this
      file and reviewed in Universal Dragon Aslam's own voice.
- [ ] Final project is submitted before 2026-07-22 00:00 UTC.
