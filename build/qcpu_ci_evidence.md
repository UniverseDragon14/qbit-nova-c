# QCPU CI Evidence Reporter

Generated UTC: 2026-07-20T10:30:46Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| CI environment | LOCAL_TERMINAL |
| Latest tag | v4.3 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |

## Hardware Fallback

| Check | Result |
|---|---|
| vcgencmd status | PASS: VCGencmd_AVAILABLE |
| Temperature | 57.1'C |
| Throttle | 0x0 |

## CI-Safe Boundary

| Check | Result |
|---|---|
| CI safe fallback | PASS: CI_SAFE_FALLBACK_READY |
| Physical QCPU | EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND |
| Virtual QCPU | PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_CI_EVIDENCE_REPORTER |
| Safety | PASS: NON_DESTRUCTIVE_CI_EVIDENCE_REPORTER |

## Verdict

QCPU CI EVIDENCE REPORTER READY
