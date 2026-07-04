#!/usr/bin/env bash
set -eu

mkdir -p build .qcpu logs

REPORT="build/qnova_public_demo_cli.md"
ENV_FILE=".qcpu/qnova_demo.env"
INPUT_LOG="build/qnova_public_demo_input.txt"

UTC_NOW="$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo unknown)"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
COMMIT="$(git log -1 --oneline 2>/dev/null || echo unknown)"
LATEST_TAG="$(git tag --sort=-version:refname 2>/dev/null | head -n 1 || echo unknown)"

if [ -z "$LATEST_TAG" ]; then
  LATEST_TAG="unknown"
fi

echo "=== QNOVA PUBLIC DEMO CLI ==="
echo "host: $HOST"
echo "arch: $ARCH"
echo "kernel: $KERNEL"
echo "tag: $LATEST_TAG"
echo "boundary: software public demo CLI, not physical quantum hardware"
echo

echo "=== STEP 1: RUN PUBLIC DEMO RUNTIME ==="
chmod +x scripts/qcpu_public_demo.sh
./scripts/qcpu_public_demo.sh > "$INPUT_LOG"

DEMO_READY="FAIL: QCPU_PUBLIC_DEMO_RUNTIME_NOT_READY"
DEMO_STATUS="FAIL: QCPU_PUBLIC_DEMO_STATUS_MISSING"
BELL_STATUS="FAIL: PUBLIC_BELL_PROOF_MISSING"
QASM_STATUS="FAIL: PUBLIC_QASM_EXPORT_MISSING"
MANIFEST_STATUS="FAIL: PUBLIC_RELEASE_MANIFEST_MISSING"
CORE_STATUS="FAIL: CORE_MUTATION_CHECK_MISSING"
SAFETY_STATUS="FAIL: PUBLIC_DEMO_SAFETY_MISSING"
BOUNDARY_STATUS="FAIL: PUBLIC_DEMO_BOUNDARY_MISSING"

if grep -Fq "QCPU PUBLIC DEMO RUNTIME READY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  DEMO_READY="PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY"
fi

if grep -Fq "QCPU_PUBLIC_DEMO_STATUS=PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md .qcpu/public_demo_runtime.env 2>/dev/null; then
  DEMO_STATUS="PASS: QCPU_PUBLIC_DEMO_STATUS_CONFIRMED"
fi

if grep -Fq "PASS: PUBLIC_BELL_PROOF_READY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  BELL_STATUS="PASS: PUBLIC_BELL_PROOF_READY"
fi

if grep -Fq "PASS: PUBLIC_QASM_EXPORT_READY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  QASM_STATUS="PASS: PUBLIC_QASM_EXPORT_READY"
fi

if grep -Fq "PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  MANIFEST_STATUS="PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY"
fi

if grep -Fq "PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO"
fi

if grep -Fq "PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  SAFETY_STATUS="PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME"
fi

if grep -Fq "PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY" "$INPUT_LOG" build/qcpu_public_demo_runtime.md 2>/dev/null; then
  BOUNDARY_STATUS="PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY"
fi

CLI_STATUS="FAIL: QNOVA_PUBLIC_DEMO_CLI_NOT_READY"
CLI_DECISION="BLOCK_PUBLIC_CLI"

if [ "$DEMO_READY" = "PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY" ] && \
   [ "$DEMO_STATUS" = "PASS: QCPU_PUBLIC_DEMO_STATUS_CONFIRMED" ] && \
   [ "$BELL_STATUS" = "PASS: PUBLIC_BELL_PROOF_READY" ] && \
   [ "$QASM_STATUS" = "PASS: PUBLIC_QASM_EXPORT_READY" ] && \
   [ "$MANIFEST_STATUS" = "PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY" ] && \
   [ "$CORE_STATUS" = "PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO" ] && \
   [ "$SAFETY_STATUS" = "PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME" ] && \
   [ "$BOUNDARY_STATUS" = "PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY" ]; then
  CLI_STATUS="PASS: QNOVA_PUBLIC_DEMO_CLI_READY"
  CLI_DECISION="ALLOW_PUBLIC_CLI"
fi

cat > "$ENV_FILE" <<ENVEOF
QNOVA_DEMO_CLI_MODE=PUBLIC_DEMO
QNOVA_DEMO_CLI_DECISION=$CLI_DECISION
QNOVA_DEMO_CLI_STATUS=$CLI_STATUS
QNOVA_DEMO_RUNTIME_READY=$DEMO_READY
QNOVA_DEMO_STATUS=$DEMO_STATUS
QNOVA_DEMO_BELL_STATUS=$BELL_STATUS
QNOVA_DEMO_QASM_STATUS=$QASM_STATUS
QNOVA_DEMO_MANIFEST_STATUS=$MANIFEST_STATUS
QNOVA_DEMO_CORE_STATUS=$CORE_STATUS
QNOVA_DEMO_SAFETY_STATUS=$SAFETY_STATUS
QNOVA_DEMO_BOUNDARY_STATUS=$BOUNDARY_STATUS
QNOVA_DEMO_TAG=$LATEST_TAG
QNOVA_DEMO_COMMIT=$COMMIT
QNOVA_DEMO_CREATED_UTC=$UTC_NOW
ENVEOF

cat > "$REPORT" <<REPOF
# QNOVA Public Demo CLI

Generated UTC: $UTC_NOW

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Latest tag | $LATEST_TAG |
| Commit | $COMMIT |

## Public CLI Summary

| Check | Result |
|---|---|
| Public demo runtime | $DEMO_READY |
| Demo status | $DEMO_STATUS |
| Bell proof | $BELL_STATUS |
| OpenQASM export | $QASM_STATUS |
| Public release manifest | $MANIFEST_STATUS |
| Core mutation check | $CORE_STATUS |
| Safety | $SAFETY_STATUS |
| Boundary | $BOUNDARY_STATUS |

## CLI Decision

| Field | Value |
|---|---|
| Decision | $CLI_DECISION |
| Status | $CLI_STATUS |

## Human Demo Command

    ./scripts/qnova_demo.sh

## Public Explanation

QBIT NOVA C is a software virtual QCPU runtime and proof chain.

It runs on a classical host.

It does not mutate hardware.

It does not claim physical quantum hardware.

## Demo Receipt

Virtual QCPU:

    $DEMO_READY

Bell proof:

    $BELL_STATUS

OpenQASM export:

    $QASM_STATUS

Release manifest:

    $MANIFEST_STATUS

Safety:

    $SAFETY_STATUS

Boundary:

    $BOUNDARY_STATUS

## Verdict

QNOVA PUBLIC DEMO CLI READY

$CLI_STATUS
REPOF

echo
echo "=== QNOVA CLI REPORT CREATED ==="
ls -lh "$REPORT" "$ENV_FILE"

echo
echo "=== QNOVA CLI VERDICT ==="
grep -F "QNOVA PUBLIC DEMO CLI READY" "$REPORT"
grep -F "PASS: QNOVA_PUBLIC_DEMO_CLI_READY" "$REPORT" "$ENV_FILE"

if [ "$CLI_STATUS" = "PASS: QNOVA_PUBLIC_DEMO_CLI_READY" ]; then
  echo
  echo "QNOVA PUBLIC DEMO CLI READY"
  echo "PASS: QNOVA_PUBLIC_DEMO_CLI_READY"
  exit 0
fi

echo
echo "$CLI_STATUS"
exit 1
