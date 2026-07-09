#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p build .qcpu

SHOTS="${QCPU_BELL_SHOTS:-20}"
MAX_SHOTS="100"

if [ "$SHOTS" -gt "$MAX_SHOTS" ]; then
  echo "ERROR: requested shots $SHOTS exceeds max $MAX_SHOTS"
  exit 1
fi

REPORT="build/qcpu_bell_shots_report.md"
ENV_FILE=".qcpu/bell_shots_report.env"
RAW_LOG="build/qcpu_bell_shots_raw.log"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m)"
KERNEL="$(uname -r)"
COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"
TAG="$(git describe --tags --abbrev=0 2>/dev/null || echo none)"
UTC="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

echo "=== QCPU BELL SHOTS REPORTER ==="
echo "shots: $SHOTS"
echo "max shots: $MAX_SHOTS"
echo "boundary: software virtual QCPU, not physical quantum hardware"

echo
echo "=== STEP 1: BUILD QNOVA IF NEEDED ==="
gcc src/qnova.c \
  src/lexer/lexer.c \
  src/parser/parser.c \
  src/compiler/compiler.c \
  src/vm/byte_vm.c \
  src/vm/state2_vm.c \
  src/quantum/state2.c \
  src/adapter/safe_adapter.c \
  -o qnova -lm

echo "PASS: QNOVA_BUILD_READY"

echo
echo "=== STEP 2: RUN BELL SHOTS ==="

COUNT_00=0
COUNT_11=0
COUNT_01=0
COUNT_10=0
COUNT_UNKNOWN=0

: > "$RAW_LOG"

for i in $(seq 1 "$SHOTS"); do
  OUT="$(./qnova examples/bell_state2.qn 2>&1 || true)"
  echo "===== SHOT $i =====" >> "$RAW_LOG"
  echo "$OUT" >> "$RAW_LOG"

  if echo "$OUT" | grep -q "|01>"; then
    COUNT_01=$((COUNT_01 + 1))
    echo "shot $i: |01> INVALID"
  elif echo "$OUT" | grep -q "|10>"; then
    COUNT_10=$((COUNT_10 + 1))
    echo "shot $i: |10> INVALID"
  elif echo "$OUT" | grep -q "|11>"; then
    COUNT_11=$((COUNT_11 + 1))
    echo "shot $i: |11> OK"
  elif echo "$OUT" | grep -q "|00>"; then
    COUNT_00=$((COUNT_00 + 1))
    echo "shot $i: |00> OK"
  else
    COUNT_UNKNOWN=$((COUNT_UNKNOWN + 1))
    echo "shot $i: UNKNOWN"
  fi
done

BAD_COUNT=$((COUNT_01 + COUNT_10 + COUNT_UNKNOWN))
GOOD_COUNT=$((COUNT_00 + COUNT_11))

echo
echo "=== STEP 3: VERDICT ==="
echo "shots: $SHOTS"
echo "|00>: $COUNT_00"
echo "|11>: $COUNT_11"
echo "|01>: $COUNT_01"
echo "|10>: $COUNT_10"
echo "unknown: $COUNT_UNKNOWN"
echo "bad count: $BAD_COUNT"

if [ "$BAD_COUNT" -eq 0 ] && [ "$GOOD_COUNT" -eq "$SHOTS" ]; then
  STATUS="PASS: QCPU_BELL_SHOTS_REPORT_READY"
  DECISION="ALLOW_BELL_SHOTS_REPORT"
  echo "$STATUS"
else
  STATUS="FAIL: QCPU_BELL_SHOTS_INVALID_OUTPUT_FOUND"
  DECISION="BLOCK_BELL_SHOTS_REPORT"
  echo "$STATUS"
  exit 1
fi

echo
echo "=== STEP 4: WRITE ENV ==="
cat > "$ENV_FILE" <<ENV
QCPU_BELL_SHOTS=$SHOTS
QCPU_BELL_COUNT_00=$COUNT_00
QCPU_BELL_COUNT_11=$COUNT_11
QCPU_BELL_COUNT_01=$COUNT_01
QCPU_BELL_COUNT_10=$COUNT_10
QCPU_BELL_COUNT_UNKNOWN=$COUNT_UNKNOWN
QCPU_BELL_BAD_COUNT=$BAD_COUNT
QCPU_BELL_GOOD_COUNT=$GOOD_COUNT
QCPU_BELL_DECISION=$DECISION
QCPU_BELL_STATUS=$STATUS
QCPU_BELL_CREATED_UTC=$UTC
ENV

echo
echo "=== STEP 5: WRITE REPORT ==="
cat > "$REPORT" <<MD
# QCPU Bell Shots Reporter

Generated UTC: $UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $COMMIT |
| Latest tag | $TAG |

## Bell Shots Summary

| Result | Count | Status |
|---|---:|---|
| |00> | $COUNT_00 | valid Bell outcome |
| |11> | $COUNT_11 | valid Bell outcome |
| |01> | $COUNT_01 | invalid for clean Bell proof |
| |10> | $COUNT_10 | invalid for clean Bell proof |
| Unknown | $COUNT_UNKNOWN | invalid / unreadable |

## Decision

| Field | Value |
|---|---|
| Shots | $SHOTS |
| Good count | $GOOD_COUNT |
| Bad count | $BAD_COUNT |
| Decision | $DECISION |
| Status | $STATUS |

## Boundary Statement

This Bell shots reporter is software-only.

It does not claim physical quantum hardware.

It checks that the clean Bell proof produces only valid correlated outcomes:
- |00>
- |11>

Invalid outcomes are blocked:
- |01>
- |10>

## Verdict

QCPU BELL SHOTS REPORTER READY

$STATUS
MD

echo
echo "=== REPORT CREATED ==="
ls -lh "$REPORT" "$ENV_FILE" "$RAW_LOG"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,120p' "$REPORT"

echo
echo "QCPU BELL SHOTS REPORTER READY"
echo "$STATUS"
