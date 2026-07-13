#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

N="${1:-3}"
RUNS="${2:-20}"

if ! [[ "$N" =~ ^[0-9]+$ ]] || [ "$N" -lt 2 ] || [ "$N" -gt 16 ]; then
  echo "ERROR: N must be an integer in the range 2..16" >&2
  exit 1
fi

if ! [[ "$RUNS" =~ ^[1-9][0-9]*$ ]]; then
  echo "ERROR: RUNS must be a positive integer" >&2
  exit 1
fi

mkdir -p build logs .qcpu

BIN="${TMPDIR:-/tmp}/qbit-nova-ghz-$$"
LOG="logs/qcpu_ghz_proof.log"
REPORT="build/qcpu_ghz_proof.md"
ENVF=".qcpu/ghz_proof.env"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m)"
KERNEL="$(uname -r)"
UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

cleanup() {
  rm -f "$BIN"
}
trap cleanup EXIT

echo "=== GHZ STATE CORRELATION PROOF ==="
echo "boundary: software virtual QCPU, not physical quantum hardware"
echo "qubits: $N"
echo "runs: $RUNS"
echo

echo "=== BUILD GHZ STATEN CORE ==="
gcc src/quantum/stateN.c -o "$BIN" -lm -std=c11
echo "PASS: GHZ_BUILD_READY"
echo

ALL0="|$(printf '0%.0s' $(seq 1 "$N"))>"
ALL1="|$(printf '1%.0s' $(seq 1 "$N"))>"

bad=0
zero_count=0
one_count=0

: > "$LOG"

for i in $(seq 1 "$RUNS"); do
  out="$("$BIN" "$N" | grep 'MEASURE' | grep -oE '\|[01]+>' || true)"

  if [ "$out" = "$ALL0" ]; then
    zero_count=$((zero_count + 1))
    echo "run $i: $out OK" | tee -a "$LOG"
  elif [ "$out" = "$ALL1" ]; then
    one_count=$((one_count + 1))
    echo "run $i: $out OK" | tee -a "$LOG"
  else
    bad=$((bad + 1))
    echo "run $i: ${out:-UNKNOWN} BAD" | tee -a "$LOG"
  fi
done

echo | tee -a "$LOG"
echo "=== RESULT ===" | tee -a "$LOG"
echo "allowed: $ALL0 and $ALL1" | tee -a "$LOG"
echo "zero count: $zero_count" | tee -a "$LOG"
echo "one count: $one_count" | tee -a "$LOG"
echo "bad count: $bad" | tee -a "$LOG"

WARNING="NONE"
if [ "$RUNS" -ge 20 ] && { [ "$zero_count" -eq 0 ] || [ "$one_count" -eq 0 ]; }; then
  WARNING="ONE_SIDED_SAMPLE_OBSERVED"
  echo "warning: $WARNING" | tee -a "$LOG"
  echo "note: valid GHZ outcomes only, but distribution should be watched" | tee -a "$LOG"
fi

if [ "$bad" -eq 0 ]; then
  STATUS="PASS: QCPU_GHZ_PROOF_READY"
  DECISION="ALLOW_GHZ_PROOF"
  echo "GHZ PROOF PASSED" | tee -a "$LOG"
  echo "Only $ALL0 and $ALL1 appeared." | tee -a "$LOG"
else
  STATUS="FAIL: QCPU_GHZ_INVALID_OUTCOME_FOUND"
  DECISION="BLOCK_GHZ_PROOF"
  echo "GHZ PROOF FAILED" | tee -a "$LOG"
fi

cat > "$ENVF" <<ENV
QCPU_GHZ_QUBITS=$N
QCPU_GHZ_RUNS=$RUNS
QCPU_GHZ_ALLOWED_ZERO=$ALL0
QCPU_GHZ_ALLOWED_ONE=$ALL1
QCPU_GHZ_ZERO_COUNT=$zero_count
QCPU_GHZ_ONE_COUNT=$one_count
QCPU_GHZ_BAD_COUNT=$bad
QCPU_GHZ_WARNING=$WARNING
QCPU_GHZ_DECISION=$DECISION
QCPU_GHZ_STATUS=$STATUS
QCPU_GHZ_CREATED_UTC=$UTC
ENV

cat > "$REPORT" <<MD
# QCPU GHZ State-N Proof

Generated UTC: $UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $COMMIT |

## GHZ Proof Result

| Field | Value |
|---|---|
| Qubits | $N |
| Runs | $RUNS |
| Allowed zero state | $ALL0 |
| Allowed one state | $ALL1 |
| Zero count | $zero_count |
| One count | $one_count |
| Bad count | $bad |
| Warning | $WARNING |
| Decision | $DECISION |
| Status | $STATUS |

## Truth Boundary

This is a software virtual QCPU proof.

It does not claim physical quantum hardware.

It validates GHZ-style state-vector correlation in C on classical hardware such as Raspberry Pi 5.

## Verdict

$STATUS
MD

echo
echo "=== GHZ REPORT CREATED ==="
ls -lh "$REPORT" "$ENVF" "$LOG"

echo
echo "=== GHZ VERDICT ==="
echo "$STATUS"

if [ "$bad" -ne 0 ]; then
  exit 1
fi
