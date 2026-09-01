# QBIT NOVA C Branch Map

Inspected on **2026-09-01**. This inventory accounts for all **58 reachable branches** at inspection time. The implementation tips below were recorded before documentation-only audit commits; those README/branch-map commits do not change the described code.

A branch name records a development checkpoint; it is not automatically a released feature or proof that the branch was merged into `main`. The commit and files on that branch are the authority.

| Branch | Inspected implementation tip | Purpose / state |
|---|---|---|
| `aslam/public-technical-identity` | `2a15b4bc7483` | Public Mintlify identity, architecture, safety and Tamil docs |
| `aslam/qbit-correctness-hotfix` | `73a57f6307ec` | Correctness test invocation fix |
| `aslam/qbit-nova-c-next-runtime` | `690c5179cd96` | QCPU Circuit VM plus OpenQASM export |
| `aslam/v4.5-expansion-collapse-kernel` | `0fb665712150` | v4.5 kernel ordering and negative-size contract fixes |
| `aslam/v4.6-virtual-qcpu-device-layer` | `412c544521c2` | Userspace qcpud socket and stalled-client timeout proof |
| `aslam/v4.7-linux-qcpu-uapi-spec` | `c44166088192` | Linux device UAPI, threat model and test plan |
| `aslam/v4.7-stage2-userspace-mock-design` | `da504ed1d791` | Stage 2 userspace mock design review resolution |
| `aslam/v4.7-stage2a-userspace-mock-core` | `57affc6a5a7c` | Stage 2A mock frontend, backend and focused proof |
| `claude/qbit-nova-testing-dzjyye` | `c014f489c555` | Portable temporary-directory fix for Termux/Pi testing |
| `codex/openai-build-week-stage2b` | `ca48ac420997` | Stage 2B qcpud adapter and Build Week evidence/docs |
| `main` | `873c64498ad8` | Current default Novakutty/QBIT NOVA C integration branch |
| `v0.4-bytecode` | `faeb9cbdece8` | Historical AST interpreter and bytecode checkpoint; README added |
| `v0.5-bytecode-vm` | `72fc501b5d85` | Historical repeat-capable bytecode VM; README added |
| `v0.5-qbc-save-load` | `035f6e118542` | Historical QBC save/load round trip; README added |
| `v0.6-safe-action` | `c95584ae1bad` | Historical simulated action allowlist; README added |
| `v0.7-adapter-contract` | `1d2726cc8fb8` | Historical safe adapter contract; README added |
| `v0.8-quantum-core` | `83b5c7ce7692` | Historical entangle simulation; README added |
| `v0.9-cnot-state-vector` | `0635cbb81ae0` | Historical CNOT simulation; README added |
| `v1.0-state-vector-core` | `4dd0d438d458` | Historical two-qbit state-vector core; README added |
| `v1.1-qn-state-vector-pipeline` | `11deda011c67` | Historical QN-to-state-vector pipeline; README added |
| `v1.2-unified-runner` | `fae9b3f42332` | Historical unified qnova runner; README added |
| `v1.3-qcpu-qmsg` | `3296c796743e` | Historical QMSG virtual packet layer; README added |
| `v1.4-qmsg-register` | `6af6fb309f4a` | Historical QMSG virtual register view; README added |
| `v1.5-readme-proof-pack` | `833f98038cff` | First public README and proof pack |
| `v1.6-github-ci` | `7a418ceb6a37` | GitHub CI proof tests |
| `v1.7-ci-badge` | `5849ee55fbf3` | CI status badge |
| `v1.8-openqasm-export` | `060946c692db` | OpenQASM exporter |
| `v1.9-bell-proof` | `1c08c7bdaf63` | Bell correlation proof |
| `v2.0-release-polish` | `007a498806cc` | First polished public release checkpoint |
| `v2.1-qasm-file-export` | `18feb83538e2` | QASM file export bridge |
| `v2.2-virtual-qcpu-boot` | `9321bb7a4967` | Software virtual-QCPU boot identity |
| `v2.3-nova-hypercube-runtime` | `6f72eb311e87` | NOVA Hypercube status layer |
| `v2.4-hypercube-snapshot` | `95e936c11b58` | Hypercube snapshot and test-script fix |
| `v2.4.1-ci-test-script-fix` | `2be357dfa36b` | CI final-marker correction |
| `v2.5-qcpu-boundary-fit-matrix` | `2b8d724c63ea` | Physical-vs-virtual boundary matrix |
| `v2.6-qcpu-noise-injection-matrix` | `709657837f60` | Synthetic noise-detection matrix |
| `v2.7-qcpu-recovery-matrix` | `972c3e7bcf22` | Noise recovery matrix |
| `v2.7.1-coderabbit-cleanup` | `27013404b6d6` | Cached Bell-summary cleanup |
| `v2.8-qcpu-fault-memory` | `d02634b02f8d` | Local fault-memory evidence |
| `v2.9-qcpu-fault-timeline` | `b0605a37a42d` | Fault timeline summary |
| `v3.0-qcpu-hardware-reality-probe` | `6177286630e5` | Read-only physical-QPU reality probe |
| `v3.1-qcpu-hardware-capability-map` | `6d1922fd5259` | Host capability map |
| `v3.2-qcpu-runtime-limit-guard` | `955275ccf2bb` | Runtime limit guard |
| `v3.3-qcpu-runtime-policy-engine` | `d02326b9333f` | Workload policy engine |
| `v3.4-qcpu-workload-admission-controller` | `c71984e67fef` | Admission controller |
| `v3.5-qcpu-workload-execution-wrapper` | `cbcf8ca69cc5` | Admitted-work execution wrapper |
| `v3.5.1-ci-hardware-fallback` | `307dd4539963` | CI hardware fallback integration |
| `v3.6-qcpu-ci-evidence-reporter` | `0c432595c335` | CI evidence reporter |
| `v3.7-qcpu-ci-evidence-gate` | `ee29326bea3e` | CI evidence gate |
| `v3.8-qcpu-release-readiness-seal` | `de830dbab4b2` | Release readiness seal |
| `v3.9-qcpu-public-release-manifest` | `ca4ac0374946` | Public release manifest |
| `v3.9.1-ci-fetch-tags` | `530d23b3e5bc` | CI tag-fetch fix |
| `v4.0-qcpu-public-demo-runtime` | `2b44c2651bbc` | Public demo runtime |
| `v4.1-qcpu-public-demo-cli` | `52669b8222e1` | Public qnova demo CLI |
| `v4.1.1-ci-no-vcgencmd-standard-fallback` | `d3c5be89a913` | CI fallback when vcgencmd is absent |
| `v4.1.2-universal-dragon-aslam-receipt` | `37419fce6656` | Creator provenance and CI recovery receipt |
| `v4.2-qnova-public-install-script` | `5f57c8a5dbd8` | Local-user installer |
| `v4.3-kimi-api-bridge` | `d3a097f5a965` | Optional Kimi API bridge |

## Lineage summary

- **v0.4–v1.4:** parser/bytecode, safe actions, early software quantum simulation, state vectors and QMSG.
- **v1.5–v2.4:** proof documentation, CI, OpenQASM, virtual-QCPU boot and Hypercube reports.
- **v2.5–v3.9:** boundary, noise/recovery, hardware probes, admission policy and evidence gates.
- **v4.0–v4.3:** public demo/install path and optional Kimi bridge.
- **v4.5–v4.7 feature branches:** bounded circuit/kernel/device work; these are not all merged into `main`.
- **Build Week branch:** Stage 2B userspace bridge and disclosure/evidence work.

## Important boundaries

- The project runs on classical hardware and provides a **software virtual QCPU**.
- Raspberry Pi hardware is the host; it is not converted into a physical quantum processor.
- OpenQASM export is an interoperability path, not evidence of physical-QPU execution.
- Generated receipts prove the code path that produced them; they are not independent external attestations unless separately signed and verified.

## Repository hygiene note

The inspected `main` tip includes tracked `.safe_patch_backup/`, `.qcpu/`, generated `build/` material and a very large `build/ci_path/` snapshot inherited from earlier backup/revert work. Those files should be reviewed in a dedicated cleanup change; this branch map does not silently delete them.

## README audit

The 12 historical branches from `v0.4-bytecode` through `v1.4-qmsg-register` contained an empty `README.md`. Each now has a branch-specific milestone README. Existing milestone READMEs were preserved.
