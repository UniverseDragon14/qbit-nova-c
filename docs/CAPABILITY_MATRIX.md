# Novakutty / QBIT NOVA C: Capability Matrix

This matrix describes the public `main` branch of **QBIT NOVA C**, the core developer-runtime project behind Novakutty. It separates source-level features and reproducible software proofs from planned product ideas.

## Available in this repository

| Area | Current capability | Evidence / boundary |
|---|---|---|
| Language toolchain | `.qn` lexer, parser, AST/compiler pipeline and bytecode VM | `src/`, `examples/`, and the full regression suite |
| Quantum-style simulation | Classical software state-vector simulation; Hadamard, CNOT, entanglement, measurement, Bell/GHZ proof scripts | `scripts/proof_bell.sh`, `scripts/proof_ghz.sh`, `tests/` |
| QMSG | Virtual bit/quantum-bit register view and text-to-bit demonstration | A software representation, not a physical quantum communication channel |
| OpenQASM | Export selected supported circuits to OpenQASM 3.0 text | Export is a circuit-description bridge; it does not submit jobs to a real QPU |
| Virtual QCPU | Local userspace runtime/protocol, `qcpud` backend, STATUS/GHZ requests, bounded I/O, mock-backend and adapter tests | Run the Stage 2B demo and full test suite; physical device/kernel integration is not implied |
| Workload admission | Policy-driven admission for supported standard work and rejection of configured heavy work | An application-level policy gate, not a replacement for OS sandboxing or system hardening |
| Verification evidence | Bell/GHZ, fault/noise/recovery, runtime-limit, release-readiness and demo receipts | Scripts generate local proof artifacts; each run must be executed to establish a fresh result |
| Host platforms | Linux on x86-64 CI and AArch64; the public userspace demo is designed to avoid root, kernel-module, device-node, GPIO and network-listener actions | Platform-specific peripheral access and optional experiments need separate evidence |

## Not shipped or not established by this repository

- A general-purpose LLM inference backend for the NOVA/EVE identity.
- A ChatGPT-equivalent conversational model, Siri-style full phone assistant, wake-word/voice pipeline, or Android-wide device-control layer.
- A physical quantum processor, quantum hardware acceleration, or a claim that a normal CPU, GPU, phone, Raspberry Pi or SD card becomes quantum hardware or RAM.
- A claim of quantum speedup. The simulator runs on conventional hardware and is subject to classical memory/runtime limits.
- A claim that every experimental kernel/device branch or every historical NOVA language line is merged into `main`.

Related repositories have distinct release tracks. In particular, **QBIT NOVA Language** currently publishes a separate experimental C-implementation specification; it is not automatically merged with this C runtime.

## Reproduce the public proof

From a Linux checkout with GCC, Bash, Python 3 and standard command-line tools:

```bash
bash scripts/qnova_build_week_demo.sh
bash scripts/test_all.sh
```

Expected final markers on a successful run:

```text
PASS: NOVAKUTTY_BUILD_WEEK_DEMO_READY
ALL QBIT NOVA TESTS PASSED
```

The demo creates local files under ignored runtime/build directories. The proof result describes the tested software path only; use the CI run and the output from your own host as the evidence for any release claim.

## Public release hygiene

`build/`, `logs/`, `.qcpu/`, `.safe_patch_backup/`, `stable_backups/`, local environment files, private-key files, compiled outputs and runtime logs must remain out of the public source tree. CI runs `scripts/check_public_hygiene.sh` to catch tracked files in these categories before the build/test stages.
