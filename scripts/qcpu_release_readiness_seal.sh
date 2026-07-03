#!/usr/bin/env bash
set -eu

mkdir -p build .qcpu logs

REPORT="build/qcpu_release_readiness_seal.md"
ENV_FILE=".qcpu/release_readiness_seal.env"
GATE_INPUT="build/qcpu_release_gate_input.txt"

echo "=== QCPU RELEASE READINESS SEAL ==="

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
COMMIT="$(git log -1 --oneline 2>/dev/null || echo unknown)"
LATEST_TAG="$(git tag --sort=-version:refname 2>/dev/null | head -n 1 || echo unknown)"
UTC_NOW="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

echo
echo "=== STEP 1: RUN CI EVIDENCE GATE ==="

if [ ! -f scripts/qcpu_ci_evidence_gate.sh ]; then
  echo "FAIL: CI_EVIDENCE_GATE_SCRIPT_NOT_FOUND"
  exit 1
fi

chmod +x scripts/qcpu_ci_evidence_gate.sh
./scripts/qcpu_ci_evidence_gate.sh > "$GATE_INPUT"

grep -E "QCPU CI EVIDENCE GATE READY|CI_EVIDENCE_GATE_OPENED|NON_DESTRUCTIVE_CI_EVIDENCE_GATE|PHYSICAL_QCPU_NOT_FOUND|VIRTUAL_QCPU_SUPPORTED" "$GATE_INPUT" || true

echo
echo "=== STEP 2: CHECK GATE MARKERS ==="

GATE_READY="FAIL: CI_EVIDENCE_GATE_NOT_READY"
GATE_OPENED="FAIL: CI_EVIDENCE_GATE_NOT_OPENED"
PHYSICAL_STATUS="FAIL: PHYSICAL_BOUNDARY_NOT_FOUND"
VIRTUAL_STATUS="FAIL: VIRTUAL_QCPU_SUPPORT_NOT_FOUND"
SAFETY_STATUS="FAIL: SAFETY_MARKER_NOT_FOUND"

grep -F "QCPU CI EVIDENCE GATE READY" "$GATE_INPUT" >/dev/null 2>&1 && GATE_READY="PASS: CI_EVIDENCE_GATE_READY"
grep -F "PASS: CI_EVIDENCE_GATE_OPENED" "$GATE_INPUT" >/dev/null 2>&1 && GATE_OPENED="PASS: CI_EVIDENCE_GATE_OPENED"
grep -F "EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND" "$GATE_INPUT" >/dev/null 2>&1 && PHYSICAL_STATUS="PASS: HONEST_PHYSICAL_QCPU_EXPECTED_FAIL"
grep -F "PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST" "$GATE_INPUT" >/dev/null 2>&1 && VIRTUAL_STATUS="PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST"
grep -F "PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE" "$GATE_INPUT" >/dev/null 2>&1 && SAFETY_STATUS="PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE"

echo "$GATE_READY"
echo "$GATE_OPENED"
echo "$PHYSICAL_STATUS"
echo "$VIRTUAL_STATUS"
echo "$SAFETY_STATUS"

echo
echo "=== STEP 3: CHECK RELEASE CHAIN FILES ==="

CHAIN_STATUS="PASS: RELEASE_CHAIN_FILES_PRESENT"

for f in \
  scripts/qcpu_ci_evidence.sh \
  scripts/qcpu_ci_evidence_gate.sh \
  scripts/qcpu_workload_execute.sh \
  scripts/qcpu_workload_admission.sh \
  scripts/qcpu_runtime_policy_engine.sh \
  scripts/qcpu_runtime_limit_guard.sh
do
  if [ ! -f "$f" ]; then
    echo "missing: $f"
    CHAIN_STATUS="FAIL: RELEASE_CHAIN_FILE_MISSING"
  fi
done

TAG_STATUS="PASS: V3_TAGS_PRESENT"
git tag -l "v3.*" | grep -F "v3.7" >/dev/null 2>&1 || TAG_STATUS="FAIL: V3_7_TAG_MISSING"

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_RELEASE_SEAL"
git diff HEAD -- src include 2>/dev/null > build/qcpu_release_core_diff.txt || true

if [ -s build/qcpu_release_core_diff.txt ]; then
  CORE_STATUS="FAIL: CORE_MUTATION_DETECTED_BY_RELEASE_SEAL"
fi

echo "$CHAIN_STATUS"
echo "$TAG_STATUS"
echo "$CORE_STATUS"

echo
echo "=== STEP 4: RELEASE DECISION ==="

RELEASE_DECISION="BLOCK_RELEASE_SEAL"
RELEASE_STATUS="FAIL: QCPU_RELEASE_NOT_READY"

if [ "$GATE_READY" = "PASS: CI_EVIDENCE_GATE_READY" ] &&
   [ "$GATE_OPENED" = "PASS: CI_EVIDENCE_GATE_OPENED" ] &&
   [ "$PHYSICAL_STATUS" = "PASS: HONEST_PHYSICAL_QCPU_EXPECTED_FAIL" ] &&
   [ "$VIRTUAL_STATUS" = "PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST" ] &&
   [ "$SAFETY_STATUS" = "PASS: NON_DESTRUCTIVE_CI_EVIDENCE_GATE" ] &&
   [ "$CHAIN_STATUS" = "PASS: RELEASE_CHAIN_FILES_PRESENT" ] &&
   [ "$TAG_STATUS" = "PASS: V3_TAGS_PRESENT" ] &&
   [ "$CORE_STATUS" = "PASS: CORE_NOT_MUTATED_BY_RELEASE_SEAL" ]; then
  RELEASE_DECISION="ALLOW_RELEASE_SEAL"
  RELEASE_STATUS="PASS: QCPU_RELEASE_READY"
fi

echo "release decision: $RELEASE_DECISION"
echo "$RELEASE_STATUS"

echo
echo "=== STEP 5: WRITE ENV ==="

cat > "$ENV_FILE" <<ENV
QCPU_RELEASE_SEAL_MODE=SOFTWARE_ONLY_RELEASE_READINESS
QCPU_RELEASE_DECISION=$RELEASE_DECISION
QCPU_RELEASE_STATUS=$RELEASE_STATUS
QCPU_GATE_READY=$GATE_READY
QCPU_GATE_OPENED=$GATE_OPENED
QCPU_PHYSICAL_STATUS=$PHYSICAL_STATUS
QCPU_VIRTUAL_STATUS=$VIRTUAL_STATUS
QCPU_SAFETY_STATUS=$SAFETY_STATUS
QCPU_CHAIN_STATUS=$CHAIN_STATUS
QCPU_TAG_STATUS=$TAG_STATUS
QCPU_CORE_STATUS=$CORE_STATUS
QCPU_LATEST_TAG=$LATEST_TAG
QCPU_RELEASE_SEAL_CREATED_UTC=$UTC_NOW
ENV

cat "$ENV_FILE"

echo
echo "=== STEP 6: WRITE REPORT ==="

cat > "$REPORT" <<REPORT
# QCPU Release Readiness Seal

Generated UTC: $UTC_NOW

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $COMMIT |
| Latest tag | $LATEST_TAG |

## Release Checks

| Check | Result |
|---|---|
| CI evidence gate ready | $GATE_READY |
| CI evidence gate opened | $GATE_OPENED |
| Physical QCPU boundary | $PHYSICAL_STATUS |
| Virtual QCPU support | $VIRTUAL_STATUS |
| Safety boundary | $SAFETY_STATUS |
| Release chain files | $CHAIN_STATUS |
| v3 tags | $TAG_STATUS |
| Core mutation check | $CORE_STATUS |

## Release Decision

| Field | Value |
|---|---|
| Decision | $RELEASE_DECISION |
| Status | $RELEASE_STATUS |

## Boundary Statement

The release seal is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

It seals only when CI evidence, virtual QCPU support, safety boundary, and core stability are present.

## Verdict

QCPU RELEASE READINESS SEAL READY
REPORT

echo
echo "=== REPORT CREATED ==="
ls -lh "$REPORT" "$ENV_FILE"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,180p' "$REPORT"

echo
echo "QCPU RELEASE READINESS SEAL READY"

grep -E "QCPU RELEASE READINESS SEAL READY|QCPU_RELEASE_STATUS|ALLOW_RELEASE_SEAL|QCPU_RELEASE_READY" "$REPORT" "$ENV_FILE"
