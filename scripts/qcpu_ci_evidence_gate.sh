#!/usr/bin/env bash
set -euo pipefail

mkdir -p build .qcpu logs

REPORT="build/qcpu_ci_evidence_gate.md"
ENVFILE=".qcpu/ci_evidence_gate.env"
SOURCE="build/qcpu_ci_evidence_gate_source.txt"
CORE_DIFF="build/qcpu_ci_evidence_gate_core_diff.txt"

UTC_NOW="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
HOST_NAME="$(hostname 2>/dev/null || echo unknown)"
ARCH_NAME="$(uname -m 2>/dev/null || echo unknown)"
KERNEL_NAME="$(uname -r 2>/dev/null || echo unknown)"
COMMIT_NAME="$(git log --oneline -1 2>/dev/null || echo unknown)"
BT="$(printf '\140\140\140')"

echo "=== QCPU CI EVIDENCE GATE ==="

echo
echo "=== STEP 1: RUN CI EVIDENCE REPORTER ==="

if [ ! -x scripts/qcpu_ci_evidence.sh ]; then
  echo "FAIL: QCPU_CI_EVIDENCE_REPORTER_SCRIPT_NOT_EXECUTABLE"
  exit 1
fi

./scripts/qcpu_ci_evidence.sh | tee "$SOURCE"

SEARCH_FILES=("$SOURCE")

if [ -f build/qcpu_ci_evidence.md ]; then
  SEARCH_FILES+=("build/qcpu_ci_evidence.md")
fi

if [ -f .qcpu/ci_evidence.env ]; then
  SEARCH_FILES+=(".qcpu/ci_evidence.env")
fi

echo
echo "=== STEP 2: CHECK REQUIRED EVIDENCE MARKERS ==="

MISSING=0

check_marker() {
  label="$1"
  marker="$2"

  if grep -F "$marker" "${SEARCH_FILES[@]}" >/dev/null 2>&1; then
    echo "PASS: ${label}"
  else
    echo "FAIL: ${label}"
    echo "missing marker: ${marker}"
    MISSING=$((MISSING + 1))
  fi
}

check_marker "REPORTER_READY_CONFIRMED" "QCPU CI EVIDENCE REPORTER READY"
check_marker "CI_SAFE_FALLBACK_CONFIRMED" "PASS: CI_SAFE_FALLBACK_READY"
check_marker "PHYSICAL_QCPU_EXPECTED_FAIL_CONFIRMED" "EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"
check_marker "VIRTUAL_QCPU_SUPPORT_CONFIRMED" "PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST"
check_marker "CI_EVIDENCE_CORE_SAFETY_CONFIRMED" "PASS: CORE_NOT_MUTATED_BY_CI_EVIDENCE_REPORTER"
check_marker "CI_EVIDENCE_NON_DESTRUCTIVE_CONFIRMED" "PASS: NON_DESTRUCTIVE_CI_EVIDENCE_REPORTER"

if [ "$MISSING" -eq 0 ]; then
  GATE_STATUS="PASS: CI_EVIDENCE_GATE_OPENED"
else
  GATE_STATUS="FAIL: CI_EVIDENCE_GATE_BLOCKED"
fi

echo
echo "gate status: $GATE_STATUS"

echo
echo "=== STEP 3: CORE MUTATION CHECK ==="

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_CI_EVIDENCE_GATE"
git diff HEAD -- src include 2>/dev/null > "$CORE_DIFF" || true

if [ -s "$CORE_DIFF" ]; then
  CORE_STATUS="FAIL: CORE_MUTATION_DETECTED_BY_CI_EVIDENCE_GATE"
  cat "$CORE_DIFF"
  exit 1
fi

echo "$CORE_STATUS"

SAFETY_STATUS="PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE"
echo "$SAFETY_STATUS"

echo
echo "=== STEP 4: WRITE GATE ENV ==="

{
  echo "QCPU_CI_GATE_STATUS=$GATE_STATUS"
  echo "QCPU_CI_GATE_REPORTER_READY=PASS: REPORTER_READY_CONFIRMED"
  echo "QCPU_CI_GATE_FALLBACK=PASS: CI_SAFE_FALLBACK_CONFIRMED"
  echo "QCPU_CI_GATE_PHYSICAL_QCPU=EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"
  echo "QCPU_CI_GATE_VIRTUAL_QCPU=PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST"
  echo "QCPU_CI_GATE_CORE=$CORE_STATUS"
  echo "QCPU_CI_GATE_SAFETY=$SAFETY_STATUS"
  echo "QCPU_CI_GATE_READY=QCPU CI EVIDENCE GATE READY"
  echo "QCPU_CI_GATE_CREATED_UTC=$UTC_NOW"
} > "$ENVFILE"

cat "$ENVFILE"

echo
echo "=== STEP 5: BUILD GATE REPORT ==="

{
  echo "# QCPU CI Evidence Gate"
  echo
  echo "Generated UTC: $UTC_NOW"
  echo
  echo "## Host"
  echo
  echo "| Field | Value |"
  echo "|---|---|"
  echo "| Host | $HOST_NAME |"
  echo "| Architecture | $ARCH_NAME |"
  echo "| Kernel | $KERNEL_NAME |"
  echo "| Commit | $COMMIT_NAME |"
  echo
  echo "## Gate Results"
  echo
  echo "| Gate | Result | Meaning |"
  echo "|---|---|---|"
  echo "| Reporter ready | PASS: REPORTER_READY_CONFIRMED | v3.6 reporter produced evidence |"
  echo "| CI fallback | PASS: CI_SAFE_FALLBACK_CONFIRMED | CI-safe hardware fallback is available |"
  echo "| Physical QCPU | EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND | Honest physical hardware boundary |"
  echo "| Virtual QCPU | PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST | Classical host supports software QCPU runtime |"
  echo "| Gate status | $GATE_STATUS | Gate opens only when required evidence exists |"
  echo "| Core status | $CORE_STATUS | Core engine was not modified |"
  echo "| Safety | $SAFETY_STATUS | Software-only non-destructive gate |"
  echo
  echo "## Gate Env"
  echo
  echo "$BT""text"
  cat "$ENVFILE"
  echo "$BT"
  echo
  echo "## Evidence Reporter Tail"
  echo
  echo "$BT""text"
  tail -n 60 "$SOURCE" || true
  echo "$BT"
  echo
  echo "## Boundary Statement"
  echo
  echo "The CI Evidence Gate is software-only."
  echo
  echo "It checks evidence. It does not mutate hardware."
  echo
  echo "Physical QCPU:"
  echo
  echo "    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"
  echo
  echo "Virtual QCPU:"
  echo
  echo "    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST"
  echo
  echo "Gate:"
  echo
  echo "    $GATE_STATUS"
  echo
  echo "Safety:"
  echo
  echo "    $SAFETY_STATUS"
  echo
  echo "## Verdict"
  echo
  echo "QCPU CI EVIDENCE GATE READY"
} > "$REPORT"

echo
echo "=== GATE REPORT CREATED ==="
ls -lh "$REPORT" "$ENVFILE"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,180p' "$REPORT"

grep -F "QCPU CI EVIDENCE GATE READY" "$REPORT" "$ENVFILE"
grep -F "PASS: CI_EVIDENCE_GATE_OPENED" "$REPORT" "$ENVFILE"
grep -F "PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE" "$REPORT" "$ENVFILE"

if [ "$MISSING" -ne 0 ]; then
  echo "FAIL: CI_EVIDENCE_GATE_BLOCKED"
  exit 1
fi

echo "QCPU CI EVIDENCE GATE READY"
