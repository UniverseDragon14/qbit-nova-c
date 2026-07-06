#!/usr/bin/env bash
set -e

mkdir -p build logs .qcpu

REPORT="build/qcpu_workload_admission.md"
ADMISSION_ENV=".qcpu/workload_admission.env"
POLICY_ENV=".qcpu/runtime_policy.env"
LIMIT_ENV=".qcpu/runtime_limits.env"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "=== QCPU WORKLOAD ADMISSION CONTROLLER ==="

echo
echo "=== STEP 1: RUN RUNTIME POLICY ENGINE ==="
./scripts/qcpu_runtime_policy_engine.sh > logs/qcpu_admission_policy_engine.log
grep -E 'QCPU RUNTIME POLICY ENGINE READY|ALLOW_STANDARD_WORKLOAD|BLOCK_HEAVY_WORKLOAD|QCPU_FINAL_POLICY|NON_DESTRUCTIVE_RUNTIME_POLICY_ENGINE' logs/qcpu_admission_policy_engine.log

if [ ! -f "$POLICY_ENV" ]; then
  echo "FAIL: RUNTIME_POLICY_ENV_NOT_FOUND"
  exit 1
fi

if [ ! -f "$LIMIT_ENV" ]; then
  echo "FAIL: RUNTIME_LIMIT_ENV_NOT_FOUND"
  exit 1
fi

echo
echo "=== STEP 2: READ POLICY AND LIMITS ==="

QCPU_MODE="$(grep '^QCPU_POLICY_MODE=' "$POLICY_ENV" | cut -d '=' -f2-)"
STANDARD_DECISION="$(grep '^QCPU_STANDARD_DECISION=' "$POLICY_ENV" | cut -d '=' -f2-)"
HEAVY_DECISION="$(grep '^QCPU_HEAVY_DECISION=' "$POLICY_ENV" | cut -d '=' -f2-)"
FINAL_POLICY="$(grep '^QCPU_FINAL_POLICY=' "$POLICY_ENV" | cut -d '=' -f2-)"
POLICY_STATUS="$(grep '^QCPU_POLICY_STATUS=' "$POLICY_ENV" | cut -d '=' -f2-)"

MAX_SHOTS="$(grep '^QCPU_MAX_SHOTS=' "$LIMIT_ENV" | cut -d '=' -f2-)"
MAX_PROOF_LOOPS="$(grep '^QCPU_MAX_PROOF_LOOPS=' "$LIMIT_ENV" | cut -d '=' -f2-)"
MAX_PARALLEL_JOBS="$(grep '^QCPU_MAX_PARALLEL_JOBS=' "$LIMIT_ENV" | cut -d '=' -f2-)"

echo "mode: $QCPU_MODE"
echo "standard decision: $STANDARD_DECISION"
echo "heavy decision: $HEAVY_DECISION"
echo "final policy: $FINAL_POLICY"
echo "max shots: $MAX_SHOTS"
echo "max proof loops: $MAX_PROOF_LOOPS"
echo "max parallel jobs: $MAX_PARALLEL_JOBS"

echo
echo "=== STEP 3: EVALUATE WORKLOAD REQUESTS ==="

STANDARD_WORKLOAD="standard_bell_proof"
STANDARD_SHOTS=20
STANDARD_LOOPS=5
STANDARD_PARALLEL=1

HEAVY_WORKLOAD="heavy_1000_shot_run"
HEAVY_SHOTS=1000
HEAVY_LOOPS=200
HEAVY_PARALLEL=1

if { [ "$FINAL_POLICY" = "ALLOW_STANDARD_WORKLOAD" ] || [ "$FINAL_POLICY" = "ALLOW_LIGHT_WORKLOAD_ONLY" ]; } &&
   [ "$STANDARD_DECISION" = "ALLOW_STANDARD_WORKLOAD" ] &&
   [ "$STANDARD_SHOTS" -le "$MAX_SHOTS" ] &&
   [ "$STANDARD_LOOPS" -le "$MAX_PROOF_LOOPS" ] &&
   [ "$STANDARD_PARALLEL" -le "$MAX_PARALLEL_JOBS" ]; then
  STANDARD_ADMISSION="ADMIT_WORKLOAD"
  STANDARD_ADMISSION_STATUS="PASS: STANDARD_WORKLOAD_ADMITTED"
else
  STANDARD_ADMISSION="REJECT_WORKLOAD"
  STANDARD_ADMISSION_STATUS="WARN: STANDARD_WORKLOAD_REJECTED"
fi

if [ "$HEAVY_DECISION" = "BLOCK_HEAVY_WORKLOAD" ] ||
   [ "$HEAVY_SHOTS" -gt "$MAX_SHOTS" ] ||
   [ "$HEAVY_LOOPS" -gt "$MAX_PROOF_LOOPS" ] ||
   [ "$HEAVY_PARALLEL" -gt "$MAX_PARALLEL_JOBS" ]; then
  HEAVY_ADMISSION="REJECT_WORKLOAD"
  HEAVY_ADMISSION_STATUS="PASS: HEAVY_WORKLOAD_REJECTED"
else
  HEAVY_ADMISSION="ADMIT_WORKLOAD"
  HEAVY_ADMISSION_STATUS="WARN: HEAVY_WORKLOAD_ADMITTED"
fi

if [ "$STANDARD_ADMISSION" = "ADMIT_WORKLOAD" ] && [ "$HEAVY_ADMISSION" = "REJECT_WORKLOAD" ]; then
  CONTROLLER_STATUS="PASS: WORKLOAD_ADMISSION_POLICY_ENFORCED"
else
  CONTROLLER_STATUS="WARN: WORKLOAD_ADMISSION_POLICY_REVIEW_REQUIRED"
fi

echo "$STANDARD_WORKLOAD: $STANDARD_ADMISSION"
echo "$STANDARD_ADMISSION_STATUS"
echo "$HEAVY_WORKLOAD: $HEAVY_ADMISSION"
echo "$HEAVY_ADMISSION_STATUS"
echo "$CONTROLLER_STATUS"

echo
echo "=== STEP 4: WRITE ADMISSION ENV ==="

cat > "$ADMISSION_ENV" <<ADMISSION
QCPU_ADMISSION_MODE=$QCPU_MODE
QCPU_FINAL_POLICY=$FINAL_POLICY
QCPU_STANDARD_WORKLOAD=$STANDARD_WORKLOAD
QCPU_STANDARD_ADMISSION=$STANDARD_ADMISSION
QCPU_STANDARD_ADMISSION_STATUS=$STANDARD_ADMISSION_STATUS
QCPU_HEAVY_WORKLOAD=$HEAVY_WORKLOAD
QCPU_HEAVY_ADMISSION=$HEAVY_ADMISSION
QCPU_HEAVY_ADMISSION_STATUS=$HEAVY_ADMISSION_STATUS
QCPU_ADMISSION_CONTROLLER_STATUS=$CONTROLLER_STATUS
QCPU_ADMISSION_CREATED_UTC=$TIME_UTC
ADMISSION

grep -E 'QCPU_STANDARD_ADMISSION|QCPU_HEAVY_ADMISSION|QCPU_ADMISSION_CONTROLLER_STATUS' "$ADMISSION_ENV"

echo
echo "=== STEP 5: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_workload_admission_core_diff.txt || true

if [ -s build/qcpu_workload_admission_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_WORKLOAD_ADMISSION"
  cat build/qcpu_workload_admission_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_WORKLOAD_ADMISSION"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_WORKLOAD_ADMISSION_CONTROLLER"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Workload Admission Controller

Generated UTC: $TIME_UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $LATEST_COMMIT |

## Policy Read

| Field | Value |
|---|---|
| QCPU mode | $QCPU_MODE |
| Final policy | $FINAL_POLICY |
| Policy status | $POLICY_STATUS |
| Max shots | $MAX_SHOTS |
| Max proof loops | $MAX_PROOF_LOOPS |
| Max parallel jobs | $MAX_PARALLEL_JOBS |

## Workload Admission Decisions

| Workload | Request | Admission | Status |
|---|---|---|---|
| $STANDARD_WORKLOAD | shots=$STANDARD_SHOTS loops=$STANDARD_LOOPS parallel=$STANDARD_PARALLEL | $STANDARD_ADMISSION | $STANDARD_ADMISSION_STATUS |
| $HEAVY_WORKLOAD | shots=$HEAVY_SHOTS loops=$HEAVY_LOOPS parallel=$HEAVY_PARALLEL | $HEAVY_ADMISSION | $HEAVY_ADMISSION_STATUS |

## Admission Env

\`\`\`text
$(cat "$ADMISSION_ENV")
\`\`\`

## Runtime Policy Engine Summary

\`\`\`text
$(grep -E 'QCPU RUNTIME POLICY ENGINE READY|ALLOW_STANDARD_WORKLOAD|BLOCK_HEAVY_WORKLOAD|QCPU_FINAL_POLICY|NON_DESTRUCTIVE_RUNTIME_POLICY_ENGINE' logs/qcpu_admission_policy_engine.log)
\`\`\`

## Boundary Statement

The admission controller is software-only.

It does not mutate hardware.

Standard workload:

    $STANDARD_ADMISSION

Heavy workload:

    $HEAVY_ADMISSION

Controller status:

    $CONTROLLER_STATUS

Core status:

    $CORE_STATUS

Safety:

    $SAFETY_STATUS

## Verdict

QCPU WORKLOAD ADMISSION CONTROLLER READY
EOF_REPORT

echo
echo "=== WORKLOAD ADMISSION REPORT CREATED ==="
ls -lh "$REPORT"
ls -lh "$ADMISSION_ENV"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,190p' "$REPORT"

echo
echo "QCPU WORKLOAD ADMISSION CONTROLLER READY"
