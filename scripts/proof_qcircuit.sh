#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p build logs .qcpu

BIN="${TMPDIR:-/tmp}/qbit-nova-circuit-$$"
LOG="logs/qcpu_circuit_vm.log"
QASM="build/qcpu_circuit_ghz3.qasm"
REPORT="build/qcpu_circuit_vm.md"
ENVF=".qcpu/circuit_vm.env"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m)"
KERNEL="$(uname -r)"
UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

cleanup() {
  rm -f "$BIN"
}
trap cleanup EXIT

echo "=== QCPU CIRCUIT VM PROOF ==="
echo "boundary: software virtual QCPU, not physical quantum hardware"

echo
echo "=== BUILD CIRCUIT VM ==="
gcc src/quantum/qcircuit.c -o "$BIN" -lm -std=c11
echo "PASS: QCIRCUIT_BUILD_READY"

echo
echo "=== RUN GHZ3 CIRCUIT ==="
"$BIN" examples/ghz3.qnc | tee "$LOG"

MEASURED="$(grep '^\[CIRCUIT\] MEASURE ' "$LOG" | tail -n 1 | grep -oE '\|[01]+>' || true)"

if [ "$MEASURED" = "|000>" ] || [ "$MEASURED" = "|111>" ]; then
  CIRCUIT_STATUS="PASS: QCIRCUIT_GHZ_MEASUREMENT_VALID"
else
  CIRCUIT_STATUS="FAIL: QCIRCUIT_GHZ_MEASUREMENT_INVALID"
fi

echo
echo "measured: ${MEASURED:-UNKNOWN}"
echo "$CIRCUIT_STATUS"

echo
echo "=== EXPORT OPENQASM ==="
"$BIN" --qasm examples/ghz3.qnc > "$QASM"
cat "$QASM"

if grep -q "OPENQASM 3.0;" "$QASM" &&
   grep -q "h q\[0\];" "$QASM" &&
   grep -q "cx q\[0\], q\[1\];" "$QASM" &&
   grep -q "cx q\[1\], q\[2\];" "$QASM"; then
  QASM_STATUS="PASS: QCIRCUIT_OPENQASM_EXPORT_READY"
else
  QASM_STATUS="FAIL: QCIRCUIT_OPENQASM_EXPORT_INVALID"
fi

echo
echo "$QASM_STATUS"

if [ "$CIRCUIT_STATUS" = "PASS: QCIRCUIT_GHZ_MEASUREMENT_VALID" ] &&
   [ "$QASM_STATUS" = "PASS: QCIRCUIT_OPENQASM_EXPORT_READY" ]; then
  STATUS="PASS: QCPU_CIRCUIT_VM_READY"
  DECISION="ALLOW_CIRCUIT_VM"
else
  STATUS="FAIL: QCPU_CIRCUIT_VM_NOT_READY"
  DECISION="BLOCK_CIRCUIT_VM"
fi

cat > "$ENVF" <<ENV
QCPU_CIRCUIT_MEASURED=$MEASURED
QCPU_CIRCUIT_STATUS=$CIRCUIT_STATUS
QCPU_CIRCUIT_QASM_STATUS=$QASM_STATUS
QCPU_CIRCUIT_DECISION=$DECISION
QCPU_CIRCUIT_FINAL_STATUS=$STATUS
QCPU_CIRCUIT_CREATED_UTC=$UTC
ENV

cat > "$REPORT" <<MD
# QCPU Circuit VM Proof

Generated UTC: $UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $COMMIT |

## Circuit VM Result

| Field | Value |
|---|---|
| Example | examples/ghz3.qnc |
| Measured result | $MEASURED |
| Circuit status | $CIRCUIT_STATUS |
| OpenQASM export | $QASM_STATUS |
| Decision | $DECISION |
| Status | $STATUS |

## Supported Gates

- h
- x
- y
- z
- s
- t
- cx
- swap
- ghz macro

## Truth Boundary

This is a C-based software virtual QCPU circuit proof.

It runs on classical hardware such as Raspberry Pi 5.

It does not claim physical quantum hardware.

It validates circuit-level software state-vector behavior and OpenQASM export.

## Verdict

$STATUS
MD

echo
echo "=== CIRCUIT REPORT CREATED ==="
ls -lh "$REPORT" "$ENVF" "$LOG" "$QASM"

echo
echo "=== CIRCUIT VERDICT ==="
echo "$STATUS"

if [ "$STATUS" != "PASS: QCPU_CIRCUIT_VM_READY" ]; then
  exit 1
fi
