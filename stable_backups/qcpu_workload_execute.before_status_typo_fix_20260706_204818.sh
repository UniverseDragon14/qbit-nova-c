#!/usr/bin/env bash
set -e

mkdir -p build logs .qcpu

REPORT="build/qcpu_workload_execution.md"
EXEC_ENV=".qcpu/workload_execution.env"
ADMISSION_ENV=".qcpu/workload_admission.env"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "=== QCPU WORKLOAD EXECUTION WRAPPER ==="

echo
echo "=== STEP 1: RUN WORKLOAD ADMISSION CONTROLLER ==="
./scripts/qcpu_workload_admission.sh > logs/qcpu_execution_admission.log
grep -E 'QCPU WORKLOAD ADMISSION CONTROLLER READY|ADMIT_WORKLOAD|REJECT_WORKLOAD|WORKLOAD_ADMISSION_POLICY_ENFORCED|NON_DESTRUCTIVE_WORKLOAD_ADMISSION_CONTROLLER' logs/qcpu_execution_admission.log

if [ ! -f "$ADMISSION_ENV" ]; then
  echo "FAIL: WORKLOAD_ADMISSION_ENV_NOT_FOUND"
  exit 1
fi

echo
echo "=== STEP 2: READ ADMISSION ==="

QCPU_MODE="$(grep '^QCPU_ADMISSION_MODE=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
FINAL_POLICY="$(grep '^QCPU_FINAL_POLICY=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
STANDARD_WORKLOAD="$(grep '^QCPU_STANDARD_WORKLOAD=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
STANDARD_ADMISSION="$(grep '^QCPU_STANDARD_ADMISSION=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
STANDARD_STATUS="$(grep '^QCPU_STANDARD_ADMISSION_STATUS=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
HEAVY_WORKLOAD="$(grep '^QCPU_HEAVY_WORKLOAD=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
HEAVY_ADMISSION="$(grep '^QCPU_HEAVY_ADMISSION=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
HEAVY_STATUS="$(grep '^QCPU_HEAVY_ADMISSION_STATUS=' "$ADMISSION_ENV" | cut -d '=' -f2-)"
ADMISSION_STATUS="$(grep '^QCPU_ADMISSION_CONTROLLER_STATUS=' "$ADMISSION_ENV" | cut -d '=' -f2-)"

echo "mode: $QCPU_MODE"
echo "final policy: $FINAL_POLICY"
echo "$STANDARD_WORKLOAD: $STANDARD_ADMISSION"
echo "$HEAVY_WORKLOAD: $HEAVY_ADMISSION"
echo "$ADMISSION_STATUS"

echo
echo "=== STEP 3: EXECUTE ADMITTED STANDARD WORKLOAD ==="

STANDARD_EXECUTION="SKIPPED"
STANDARD_EXECUTION_STATUS="WARN: STANDARD_WORKLOAD_NOT_EXECUTED"

if [ "$STANDARD_ADMISSION" = "ADMIT_WORKLOAD" ]; then
  ./scripts/proof_bell.sh 10 > logs/qcpu_execution_standard_bell.log

  if grep -q "BELL PROOF PASSED" logs/qcpu_execution_standard_bell.log &&
     grep -q "bad count: 0" logs/qcpu_execution_standard_bell.log; then
    STANDARD_EXECUTION="EXECUTED"
    STANDARD_EXECUTION_STATUS="PASS: STANDARD_WORKLOAD_EXECUTED"
  else
    STANDARD_EXECUTION="FAILED"
    STANDARD_EXECUTION_STATUS="FAIL: STANDARD_WORKLOAD_EXECUTION_FAILED"
    cat logs/qcpu_execution_standard_bell.log
    exit 1
  fi
else
  echo "standard workload was not admitted" > logs/qcpu_execution_standard_bell.log
fi

echo "$STANDARD_EXECUTION_STATUS"

echo
echo "=== STEP 4: BLOCK REJECTED HEAVY WORKLOAD ==="

HEAVY_EXECUTION="SKIPPED"
HEAVY_EXECUTION_STATUS="WARN: HEAVY_WORKLOAD_STATE_UNKNOWN"

if [ "$HEAVY_ADMISSION" = "REJECT_WORKLOAD" ]; then
  HEAVY_EXECUTION="BLOCKED"
  HEAVY_EXECUTION_STATUS="PASS: HEAVY_WORKLOAD_NOT_EXECUTED"
  {
    echo "heavy workload rejected by admission controller"
    echo "workload: $HEAVY_WORKLOAD"
    echo "decision: $HEAVY_ADMISSION"
    echo "execution: $HEAVY_EXECUTION"
  } > logs/qcpu_execution_heavy_blocked.log
else
  HEAVY_EXECUTION="EXECUTED_UNEXPECTEDLY"
  HEAVY_EXECUTION_STATUS="FAIL: HEAVY_WORKLOAD_WOULD_EXECUTE"
  echo "heavy workload was not rejected; refusing wrapper pass" > logs/qcpu_execution_heavy_blocked.log
  exit 1
fi

echo "$HEAVY_EXECUTION_STATUS"

echo
echo "=== STEP 5: FINAL EXECUTION VERDICT ==="

if [ "$STANDARD_EXECUTION" = "EXECUTED" ] && [ "$HEAVY_EXECUTION" = "BLOCKED" ]; then
  WRAPPER_STATUS="PASS: WORKLOAD_EXECUTION_POLICY_ENFORCED"
else
  WRAPPER_STATUS="FAIL: WORKLOAD_EXECUTION_POLICY_NOT_ENFORCED"
  exit 1
fi

echo "$WRAPPER_STATUS"

echo
echo "=== STEP 6: WRITE EXECUTION ENV ==="

cat > "$EXEC_ENV" <<EXECUTION
QCPU_EXECUTION_MODE=$QCPU_MODE
QCPU_FINAL_POLICY=$FINAL_POLICY
QCPU_STANDARD_WORKLOAD=$STANDARD_WORKLOAD
QCPU_STANDARD_ADMISSION=$STANDARD_ADMISSION
QCPU_STANDARD_EXECUTION=$STANDARD_EXECUTION
QCPU_STANDARD_EXECUTION_STATUS=$STANDARD_EXECUTION_STATUS
QCPU_HEAVY_WORKLOAD=$HEAVY_WORKLOAD
QCPU_HEAVY_ADMISSION=$HEAVY_ADMISSION
QCPU_HEAVY_EXECUTION=$HEAVY_EXECUTION
QCPU_HEAVY_EXECUTION_STATUS=$HEAVY_EXECUTION_STATUS
QCPU_EXECUTION_WRAPPER_STATUS=$WRAPPER_STATUS
QCPU_EXECUTION_CREATED_UTC=$TIME_UTC
EXECUTION

grep -E 'QCPU_STANDARD_EXECUTION|QCPU_HEAVY_EXECUTION|QCPU_EXECUTION_WRAPPER_STATUS' "$EXEC_ENV"

echo
echo "=== STEP 7: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_workload_execution_core_diff.txt || true

if [ -s build/qcpu_workload_execution_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_WORKLOAD_EXECUTION"
  cat build/qcpu_workload_execution_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_WORKLOAD_EXECUTION"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_WORKLOAD_EXECUTION_WRAPPER"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Workload Execution Wrapper

Generated UTC: $TIME_UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $LATEST_COMMIT |

## Admission Read

| Field | Value |
|---|---|
| QCPU mode | $QCPU_MODE |
| Final policy | $FINAL_POLICY |
| Admission status | $ADMISSION_STATUS |
| Standard workload | $STANDARD_WORKLOAD |
| Standard admission | $STANDARD_ADMISSION |
| Heavy workload | $HEAVY_WORKLOAD |
| Heavy admission | $HEAVY_ADMISSION |

## Execution Decisions

| Workload | Admission | Execution | Status |
|---|---|---|---|
| $STANDARD_WORKLOAD | $STANDARD_ADMISSION | $STANDARD_EXECUTION | $STANDARD_EXECUTION_STATUS |
| $HEAVY_WORKLOAD | $HEAVY_ADMISSION | $HEAVY_EXECUTION | $HEAVY_EXECUTION_STATUS |

## Execution Env

\`\`\`text
$(cat "$EXEC_ENV")
\`\`\`

## Standard Workload Proof Summary

\`\`\`text
$(grep -E 'bad count: 0|BELL PROOF PASSED|run [0-9]+:|Only \|00> and \|11>' logs/qcpu_execution_standard_bell.log | head -n 30)
\`\`\`

## Heavy Workload Block Log

\`\`\`text
$(cat logs/qcpu_execution_heavy_blocked.log)
\`\`\`

## Boundary Statement

The execution wrapper is software-only.

It executes admitted workloads only.

Standard workload:

    $STANDARD_EXECUTION_STATUS

Heavy workload:

    $HEAVY_EXECUTION_STATUS

Wrapper status:

    $WRAPPER_STATUS

Core status:

    $CORE_STATUS

Safety:

    $SAFETY_STATUS

## Verdict

QCPU WORKLOAD EXECUTION WRAPPER READY
EOF_REPORT

echo
echo "=== WORKLOAD EXECUTION REPORT CREATED ==="
ls -lh "$REPORT"
ls -lh "$EXEC_ENV"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,210p' "$REPORT"

echo
echo "QCPU WORKLOAD EXECUTION WRAPPER READY"
