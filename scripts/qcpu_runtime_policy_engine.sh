#!/usr/bin/env bash
set -e

mkdir -p build logs .qcpu

REPORT="build/qcpu_runtime_policy_engine.md"
POLICY_ENV=".qcpu/runtime_policy.env"
LIMIT_ENV=".qcpu/runtime_limits.env"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "=== QCPU RUNTIME POLICY ENGINE ==="

echo
echo "=== STEP 1: RUN RUNTIME LIMIT GUARD ==="
./scripts/qcpu_runtime_limit_guard.sh > logs/qcpu_policy_runtime_guard.log
grep -E 'QCPU RUNTIME LIMIT GUARD READY|QCPU_RUNTIME_MODE|QCPU_MAX_SHOTS|QCPU_LIMIT_GUARD_STATUS|NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD' logs/qcpu_policy_runtime_guard.log

if [ ! -f "$LIMIT_ENV" ]; then
  echo "FAIL: RUNTIME_LIMIT_ENV_NOT_FOUND"
  exit 1
fi

echo
echo "=== STEP 2: READ LIMITS ==="

QCPU_MODE="$(grep '^QCPU_RUNTIME_MODE=' "$LIMIT_ENV" | cut -d '=' -f2-)"
MAX_SHOTS="$(grep '^QCPU_MAX_SHOTS=' "$LIMIT_ENV" | cut -d '=' -f2-)"
MAX_PROOF_LOOPS="$(grep '^QCPU_MAX_PROOF_LOOPS=' "$LIMIT_ENV" | cut -d '=' -f2-)"
MAX_PARALLEL_JOBS="$(grep '^QCPU_MAX_PARALLEL_JOBS=' "$LIMIT_ENV" | cut -d '=' -f2-)"
TEMP_VALUE="$(grep '^QCPU_TEMP_VALUE=' "$LIMIT_ENV" | cut -d '=' -f2-)"
THROTTLE_VALUE="$(grep '^QCPU_THROTTLE_VALUE=' "$LIMIT_ENV" | cut -d '=' -f2-)"
LIMIT_GUARD_STATUS="$(grep '^QCPU_LIMIT_GUARD_STATUS=' "$LIMIT_ENV" | cut -d '=' -f2-)"

echo "mode: $QCPU_MODE"
echo "max shots: $MAX_SHOTS"
echo "max proof loops: $MAX_PROOF_LOOPS"
echo "max parallel jobs: $MAX_PARALLEL_JOBS"
echo "temp: $TEMP_VALUE"
echo "throttle: $THROTTLE_VALUE"

echo
echo "=== STEP 3: POLICY DECISIONS ==="

STANDARD_SHOTS=20
STANDARD_LOOPS=5
HEAVY_SHOTS=1000
HEAVY_LOOPS=200

if [ "$MAX_PARALLEL_JOBS" -ge 1 ] && [ "$STANDARD_SHOTS" -le "$MAX_SHOTS" ] && [ "$STANDARD_LOOPS" -le "$MAX_PROOF_LOOPS" ]; then
  STANDARD_DECISION="ALLOW_STANDARD_WORKLOAD"
  STANDARD_STATUS="PASS: STANDARD_WORKLOAD_ALLOWED"
else
  STANDARD_DECISION="BLOCK_STANDARD_WORKLOAD"
  STANDARD_STATUS="WARN: STANDARD_WORKLOAD_BLOCKED"
fi

if [ "$HEAVY_SHOTS" -le "$MAX_SHOTS" ] && [ "$HEAVY_LOOPS" -le "$MAX_PROOF_LOOPS" ] && [ "$MAX_PARALLEL_JOBS" -ge 1 ]; then
  HEAVY_DECISION="ALLOW_HEAVY_WORKLOAD"
  HEAVY_STATUS="WARN: HEAVY_WORKLOAD_ALLOWED"
else
  HEAVY_DECISION="BLOCK_HEAVY_WORKLOAD"
  HEAVY_STATUS="PASS: HEAVY_WORKLOAD_BLOCKED"
fi

if [ "$QCPU_MODE" = "REST_REQUIRED_MODE" ]; then
  FINAL_POLICY="BLOCK_HEAVY_WORKLOAD"
  FINAL_STATUS="WARN: REST_REQUIRED_POLICY_ACTIVE"
elif [ "$QCPU_MODE" = "CONSERVATIVE_VIRTUAL_QCPU_MODE" ]; then
  FINAL_POLICY="ALLOW_LIGHT_WORKLOAD_ONLY"
  FINAL_STATUS="WARN: CONSERVATIVE_POLICY_ACTIVE"
else
  FINAL_POLICY="ALLOW_STANDARD_WORKLOAD"
  FINAL_STATUS="PASS: STANDARD_POLICY_ACTIVE"
fi

echo "$STANDARD_STATUS"
echo "$HEAVY_STATUS"
echo "$FINAL_STATUS"

echo
echo "=== STEP 4: WRITE POLICY ENV ==="

cat > "$POLICY_ENV" <<POLICY
QCPU_POLICY_MODE=$QCPU_MODE
QCPU_STANDARD_DECISION=$STANDARD_DECISION
QCPU_HEAVY_DECISION=$HEAVY_DECISION
QCPU_FINAL_POLICY=$FINAL_POLICY
QCPU_STANDARD_SHOTS=$STANDARD_SHOTS
QCPU_STANDARD_LOOPS=$STANDARD_LOOPS
QCPU_HEAVY_SHOTS=$HEAVY_SHOTS
QCPU_HEAVY_LOOPS=$HEAVY_LOOPS
QCPU_POLICY_CREATED_UTC=$TIME_UTC
QCPU_POLICY_STATUS=$FINAL_STATUS
POLICY

grep -E 'QCPU_STANDARD_DECISION|QCPU_HEAVY_DECISION|QCPU_FINAL_POLICY|QCPU_POLICY_STATUS' "$POLICY_ENV"

echo
echo "=== STEP 5: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_runtime_policy_core_diff.txt || true

if [ -s build/qcpu_runtime_policy_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_RUNTIME_POLICY_ENGINE"
  cat build/qcpu_runtime_policy_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_RUNTIME_POLICY_ENGINE"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_RUNTIME_POLICY_ENGINE"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Runtime Policy Engine

Generated UTC: $TIME_UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $LATEST_COMMIT |

## Runtime Limits Read

| Limit | Value |
|---|---|
| Runtime mode | $QCPU_MODE |
| Max shots | $MAX_SHOTS |
| Max proof loops | $MAX_PROOF_LOOPS |
| Max parallel jobs | $MAX_PARALLEL_JOBS |
| Temperature | $TEMP_VALUE |
| Throttle | $THROTTLE_VALUE |
| Limit guard status | $LIMIT_GUARD_STATUS |

## Policy Decisions

| Workload | Request | Decision | Status |
|---|---|---|---|
| Standard workload | shots=$STANDARD_SHOTS loops=$STANDARD_LOOPS | $STANDARD_DECISION | $STANDARD_STATUS |
| Heavy workload | shots=$HEAVY_SHOTS loops=$HEAVY_LOOPS | $HEAVY_DECISION | $HEAVY_STATUS |
| Final policy | mode=$QCPU_MODE | $FINAL_POLICY | $FINAL_STATUS |

## Policy Env

\`\`\`text
$(cat "$POLICY_ENV")
\`\`\`

## Runtime Limit Guard Summary

\`\`\`text
$(grep -E 'QCPU RUNTIME LIMIT GUARD READY|QCPU_RUNTIME_MODE|QCPU_MAX_SHOTS|QCPU_LIMIT_GUARD_STATUS|NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD' logs/qcpu_policy_runtime_guard.log)
\`\`\`

## Boundary Statement

The policy engine is software-only.

It does not mutate hardware.

Standard workload decision:

    $STANDARD_DECISION

Heavy workload decision:

    $HEAVY_DECISION

Final policy:

    $FINAL_POLICY

Core status:

    $CORE_STATUS

Safety:

    $SAFETY_STATUS

## Verdict

QCPU RUNTIME POLICY ENGINE READY
EOF_REPORT

echo
echo "=== RUNTIME POLICY REPORT CREATED ==="
ls -lh "$REPORT"
ls -lh "$POLICY_ENV"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,180p' "$REPORT"

echo
echo "QCPU RUNTIME POLICY ENGINE READY"
