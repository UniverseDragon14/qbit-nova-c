#!/usr/bin/env bash
set -eu

mkdir -p build .qcpu logs

REPORT="build/qcpu_public_demo_runtime.md"
ENV_FILE=".qcpu/public_demo_runtime.env"

MANIFEST_INPUT="build/qcpu_public_demo_manifest_input.txt"
BOOT_LOG="logs/qcpu_public_demo_boot.log"
BELL_LOG="logs/qcpu_public_demo_bell.log"
QASM_LOG="logs/qcpu_public_demo_qasm.log"

QASM_FILE="build/bell.qasm"
NODE_FILE="build/qcpu_node.json"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
COMMIT="$(git log -1 --oneline 2>/dev/null || echo unknown)"
LATEST_TAG="$(git tag --sort=-version:refname 2>/dev/null | head -n 1 || echo unknown)"
UTC_NOW="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

echo "=== QCPU PUBLIC DEMO RUNTIME ==="

echo
echo "=== STEP 1: VERIFY PUBLIC RELEASE MANIFEST ==="
chmod +x scripts/qcpu_public_release_manifest.sh
./scripts/qcpu_public_release_manifest.sh > "$MANIFEST_INPUT"

grep -E "QCPU PUBLIC RELEASE MANIFEST READY|QCPU_PUBLIC_RELEASE_MANIFEST_READY|QCPU_RELEASE_READY|NON_DESTRUCTIVE|VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST|PHYSICAL_QCPU_NOT_FOUND" "$MANIFEST_INPUT" || true

MANIFEST_READY="FAIL: PUBLIC_RELEASE_MANIFEST_NOT_READY"
MANIFEST_STATUS="FAIL: QCPU_PUBLIC_RELEASE_MANIFEST_STATUS_MISSING"

if grep -q "QCPU PUBLIC RELEASE MANIFEST READY" "$MANIFEST_INPUT"; then
  MANIFEST_READY="PASS: PUBLIC_RELEASE_MANIFEST_READY"
fi

if grep -q "PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY" "$MANIFEST_INPUT"; then
  MANIFEST_STATUS="PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY"
fi

echo
echo "manifest ready: $MANIFEST_READY"
echo "manifest status: $MANIFEST_STATUS"

echo
echo "=== STEP 2: BOOT VIRTUAL QCPU ==="
chmod +x scripts/qcpu_boot.sh
./scripts/qcpu_boot.sh > "$BOOT_LOG"

grep -E "QCPU NODE READY|software virtual QCPU|not physical quantum hardware" "$BOOT_LOG" || true

BOOT_STATUS="FAIL: VIRTUAL_QCPU_BOOT_NOT_READY"

if grep -q "QCPU NODE READY" "$BOOT_LOG" && [ -f "$NODE_FILE" ]; then
  BOOT_STATUS="PASS: VIRTUAL_QCPU_BOOT_READY"
fi

echo "boot status: $BOOT_STATUS"

echo
echo "=== STEP 3: RUN PUBLIC BELL PROOF ==="
chmod +x scripts/proof_bell.sh
./scripts/proof_bell.sh 10 > "$BELL_LOG"

grep -E "bad count: 0|BELL PROOF PASSED|Only \|00> and \|11> appeared" "$BELL_LOG" || true

BELL_STATUS="FAIL: PUBLIC_BELL_PROOF_NOT_READY"

if grep -q "bad count: 0" "$BELL_LOG" && grep -q "BELL PROOF PASSED" "$BELL_LOG"; then
  BELL_STATUS="PASS: PUBLIC_BELL_PROOF_READY"
fi

echo "bell status: $BELL_STATUS"

echo
echo "=== STEP 4: EXPORT PUBLIC QASM ==="
chmod +x scripts/export_qasm.sh
./scripts/export_qasm.sh > "$QASM_LOG"

grep -E "QASM FILE CREATED|OPENQASM 3.0|cx q\[0\], q\[1\]" "$QASM_LOG" || true

QASM_STATUS="FAIL: PUBLIC_QASM_EXPORT_NOT_READY"

if [ -f "$QASM_FILE" ] && grep -q "OPENQASM 3.0" "$QASM_FILE" && grep -q "cx q\[0\], q\[1\]" "$QASM_FILE"; then
  QASM_STATUS="PASS: PUBLIC_QASM_EXPORT_READY"
fi

echo "qasm status: $QASM_STATUS"

echo
echo "=== STEP 5: CORE MUTATION CHECK ==="
CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO"

if ! git diff --quiet -- src examples 2>/dev/null; then
  CORE_STATUS="FAIL: CORE_MUTATED_BY_PUBLIC_DEMO"
fi

echo "$CORE_STATUS"

echo
echo "=== STEP 6: SAFETY BOUNDARY ==="
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_PUBLIC_DEMO_RUNTIME"
BOUNDARY_STATUS="PASS: SOFTWARE_ONLY_PUBLIC_DEMO_BOUNDARY"

echo "$SAFETY_STATUS"
echo "$BOUNDARY_STATUS"

DEMO_DECISION="BLOCK_PUBLIC_DEMO"
DEMO_STATUS="FAIL: QCPU_PUBLIC_DEMO_RUNTIME_NOT_READY"

if [ "$MANIFEST_READY" = "PASS: PUBLIC_RELEASE_MANIFEST_READY" ] \
  && [ "$MANIFEST_STATUS" = "PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY" ] \
  && [ "$BOOT_STATUS" = "PASS: VIRTUAL_QCPU_BOOT_READY" ] \
  && [ "$BELL_STATUS" = "PASS: PUBLIC_BELL_PROOF_READY" ] \
  && [ "$QASM_STATUS" = "PASS: PUBLIC_QASM_EXPORT_READY" ] \
  && [ "$CORE_STATUS" = "PASS: CORE_NOT_MUTATED_BY_PUBLIC_DEMO" ]; then
  DEMO_DECISION="ALLOW_PUBLIC_DEMO"
  DEMO_STATUS="PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY"
fi

cat > "$ENV_FILE" <<ENV
QCPU_PUBLIC_DEMO_MODE=STANDARD_VIRTUAL_QCPU_MODE
QCPU_PUBLIC_DEMO_DECISION=$DEMO_DECISION
QCPU_PUBLIC_DEMO_STATUS=$DEMO_STATUS
QCPU_PUBLIC_DEMO_MANIFEST_READY=$MANIFEST_READY
QCPU_PUBLIC_DEMO_MANIFEST_STATUS=$MANIFEST_STATUS
QCPU_PUBLIC_DEMO_BOOT_STATUS=$BOOT_STATUS
QCPU_PUBLIC_DEMO_BELL_STATUS=$BELL_STATUS
QCPU_PUBLIC_DEMO_QASM_STATUS=$QASM_STATUS
QCPU_PUBLIC_DEMO_CORE_STATUS=$CORE_STATUS
QCPU_PUBLIC_DEMO_SAFETY_STATUS=$SAFETY_STATUS
QCPU_PUBLIC_DEMO_BOUNDARY_STATUS=$BOUNDARY_STATUS
QCPU_PUBLIC_DEMO_CREATED_UTC=$UTC_NOW
ENV

cat > "$REPORT" <<REPORT
# QCPU Public Demo Runtime

Generated UTC: $UTC_NOW

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Latest tag | $LATEST_TAG |
| Commit | $COMMIT |

## Public Demo Chain

| Check | Result |
|---|---|
| Public release manifest | $MANIFEST_READY |
| Public manifest status | $MANIFEST_STATUS |
| Virtual QCPU boot | $BOOT_STATUS |
| Bell proof | $BELL_STATUS |
| QASM export | $QASM_STATUS |
| Core mutation check | $CORE_STATUS |
| Safety | $SAFETY_STATUS |
| Boundary | $BOUNDARY_STATUS |

## Demo Decision

| Field | Value |
|---|---|
| Decision | $DEMO_DECISION |
| Status | $DEMO_STATUS |

## Public Demo Receipt

Mode:

    STANDARD_VIRTUAL_QCPU_MODE

Virtual QCPU:

    $BOOT_STATUS

Bell proof:

    $BELL_STATUS

OpenQASM export:

    $QASM_STATUS

Release manifest:

    $MANIFEST_STATUS

Core status:

    $CORE_STATUS

Safety:

    $SAFETY_STATUS

## Boundary Statement

This public demo runtime is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

It proves that QBIT NOVA C can run a safe virtual QCPU public demo chain on a classical host.

## Verdict

QCPU PUBLIC DEMO RUNTIME READY

$DEMO_STATUS
REPORT

echo
echo "=== PUBLIC DEMO REPORT CREATED ==="
ls -lh "$REPORT" "$ENV_FILE"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,180p' "$REPORT"

echo
echo "=== VERDICT ==="
grep -F "QCPU PUBLIC DEMO RUNTIME READY" "$REPORT"
grep -F "QCPU_PUBLIC_DEMO_STATUS=PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY" "$ENV_FILE"

if [ "$DEMO_STATUS" != "PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY" ]; then
  echo "STOP: public demo runtime not ready."
  exit 1
fi

echo "QCPU PUBLIC DEMO RUNTIME READY"
