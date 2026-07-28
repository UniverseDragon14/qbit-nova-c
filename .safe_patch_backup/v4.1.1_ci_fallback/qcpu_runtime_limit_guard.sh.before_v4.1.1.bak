#!/usr/bin/env bash
set -e

mkdir -p build logs .qcpu

REPORT="build/qcpu_runtime_limit_guard.md"
LIMIT_ENV=".qcpu/runtime_limits.env"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

TEMP_VALUE="unknown"
THROTTLE_VALUE="unknown"
TEMP_NUM=0

if command -v vcgencmd >/dev/null 2>&1; then
  TEMP_VALUE="$(vcgencmd measure_temp 2>/dev/null | sed 's/temp=//' || echo unknown)"
  THROTTLE_VALUE="$(vcgencmd get_throttled 2>/dev/null | sed 's/throttled=//' || echo unknown)"
else
  if [ -f /sys/class/thermal/thermal_zone0/temp ]; then
    RAW_TEMP="$(cat /sys/class/thermal/thermal_zone0/temp)"
    TEMP_VALUE="$((RAW_TEMP / 1000))C"
  fi
fi

TEMP_NUM="$(printf '%s' "$TEMP_VALUE" | grep -oE '^[0-9]+' || echo 0)"
MEM_FREE_MB="$(awk '/MemAvailable:/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 0)"
DISK_FREE_MB="$(df -Pm / 2>/dev/null | awk 'NR==2 {print $4}' || echo 0)"
CPU_CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1)"

echo "=== QCPU RUNTIME LIMIT GUARD ==="

echo
echo "=== STEP 1: RUN HARDWARE CAPABILITY MAP ==="
./scripts/qcpu_hardware_capability_map.sh > logs/qcpu_runtime_guard_capability.log
grep -E 'QCPU HARDWARE CAPABILITY MAP READY|STANDARD_VIRTUAL_QCPU_MODE|CONSERVATIVE_VIRTUAL_QCPU_MODE|REST_REQUIRED_MODE|PASS: READ_ONLY_CAPABILITY_MAP' logs/qcpu_runtime_guard_capability.log

echo
echo "=== STEP 2: COMPUTE LIMITS ==="

if [ "$TEMP_NUM" -lt 65 ] && [ "$THROTTLE_VALUE" = "0x0" ] && [ "$MEM_FREE_MB" -ge 1024 ] && [ "$DISK_FREE_MB" -ge 1024 ]; then
  QCPU_MODE="STANDARD_VIRTUAL_QCPU_MODE"
  MAX_SHOTS=100
  MAX_PROOF_LOOPS=20
  MAX_PARALLEL_JOBS=1
  GUARD_STATUS="PASS: STANDARD_RUNTIME_LIMITS_ACTIVE"
elif [ "$TEMP_NUM" -lt 75 ] && [ "$MEM_FREE_MB" -ge 512 ]; then
  QCPU_MODE="CONSERVATIVE_VIRTUAL_QCPU_MODE"
  MAX_SHOTS=30
  MAX_PROOF_LOOPS=10
  MAX_PARALLEL_JOBS=1
  GUARD_STATUS="WARN: CONSERVATIVE_RUNTIME_LIMITS_ACTIVE"
else
  QCPU_MODE="REST_REQUIRED_MODE"
  MAX_SHOTS=5
  MAX_PROOF_LOOPS=3
  MAX_PARALLEL_JOBS=0
  GUARD_STATUS="WARN: REST_REQUIRED_BEFORE_HEAVY_QCPU_WORK"
fi

if [ "$THROTTLE_VALUE" = "0x0" ]; then
  THROTTLE_STATUS="PASS: NO_THROTTLING"
else
  THROTTLE_STATUS="WARN: THROTTLE_STATE_$THROTTLE_VALUE"
fi

if [ "$TEMP_NUM" -lt 65 ]; then
  TEMP_STATUS="PASS: TEMP_SAFE_FOR_RUNTIME"
elif [ "$TEMP_NUM" -lt 75 ]; then
  TEMP_STATUS="WARN: TEMP_WARM_RUNTIME_LIMITED"
else
  TEMP_STATUS="WARN: TEMP_HOT_REST_REQUIRED"
fi

if [ "$MEM_FREE_MB" -ge 1024 ]; then
  MEMORY_STATUS="PASS: MEMORY_AVAILABLE_FOR_RUNTIME"
else
  MEMORY_STATUS="WARN: MEMORY_LOW_RUNTIME_LIMITED"
fi

if [ "$DISK_FREE_MB" -ge 1024 ]; then
  DISK_STATUS="PASS: DISK_AVAILABLE_FOR_RUNTIME"
else
  DISK_STATUS="WARN: DISK_LOW_RUNTIME_LIMITED"
fi

echo "mode: $QCPU_MODE"
echo "max shots: $MAX_SHOTS"
echo "max proof loops: $MAX_PROOF_LOOPS"
echo "max parallel jobs: $MAX_PARALLEL_JOBS"
echo "$GUARD_STATUS"
echo "$TEMP_STATUS"
echo "$THROTTLE_STATUS"
echo "$MEMORY_STATUS"
echo "$DISK_STATUS"

echo
echo "=== STEP 3: WRITE LIMIT ENV ==="

cat > "$LIMIT_ENV" <<LIMITS
QCPU_RUNTIME_MODE=$QCPU_MODE
QCPU_MAX_SHOTS=$MAX_SHOTS
QCPU_MAX_PROOF_LOOPS=$MAX_PROOF_LOOPS
QCPU_MAX_PARALLEL_JOBS=$MAX_PARALLEL_JOBS
QCPU_TEMP_VALUE=$TEMP_VALUE
QCPU_THROTTLE_VALUE=$THROTTLE_VALUE
QCPU_MEMORY_FREE_MB=$MEM_FREE_MB
QCPU_DISK_FREE_MB=$DISK_FREE_MB
QCPU_LIMIT_CREATED_UTC=$TIME_UTC
QCPU_LIMIT_GUARD_STATUS=$GUARD_STATUS
LIMITS

grep -E 'QCPU_RUNTIME_MODE|QCPU_MAX_SHOTS|QCPU_LIMIT_GUARD_STATUS' "$LIMIT_ENV"

echo
echo "=== STEP 4: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_runtime_limit_core_diff.txt || true

if [ -s build/qcpu_runtime_limit_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_RUNTIME_LIMIT_GUARD"
  cat build/qcpu_runtime_limit_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_RUNTIME_LIMIT_GUARD"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Runtime Limit Guard

Generated UTC: $TIME_UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| CPU cores | $CPU_CORES |
| Commit | $LATEST_COMMIT |

## Runtime Limits

| Limit | Value |
|---|---|
| Runtime mode | $QCPU_MODE |
| Max shots | $MAX_SHOTS |
| Max proof loops | $MAX_PROOF_LOOPS |
| Max parallel jobs | $MAX_PARALLEL_JOBS |
| Temperature | $TEMP_VALUE |
| Throttle | $THROTTLE_VALUE |
| Memory free MB | $MEM_FREE_MB |
| Disk free MB | $DISK_FREE_MB |

## Guard Status

| Check | Result |
|---|---|
| Guard | $GUARD_STATUS |
| Temperature | $TEMP_STATUS |
| Throttle | $THROTTLE_STATUS |
| Memory | $MEMORY_STATUS |
| Disk | $DISK_STATUS |
| Core | $CORE_STATUS |
| Safety | $SAFETY_STATUS |

## Runtime Limit Env

\`\`\`text
$(cat "$LIMIT_ENV")
\`\`\`

## Capability Map Summary

\`\`\`text
$(grep -E 'QCPU HARDWARE CAPABILITY MAP READY|STANDARD_VIRTUAL_QCPU_MODE|CONSERVATIVE_VIRTUAL_QCPU_MODE|REST_REQUIRED_MODE|PASS: READ_ONLY_CAPABILITY_MAP' logs/qcpu_runtime_guard_capability.log)
\`\`\`

## Boundary Statement

The runtime guard is software-only.

It does not mutate hardware.

Recommended runtime mode:

    $QCPU_MODE

Guard status:

    $GUARD_STATUS

Safety:

    $SAFETY_STATUS

## Verdict

QCPU RUNTIME LIMIT GUARD READY
EOF_REPORT

echo
echo "=== RUNTIME LIMIT REPORT CREATED ==="
ls -lh "$REPORT"
ls -lh "$LIMIT_ENV"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,170p' "$REPORT"

echo
echo "QCPU RUNTIME LIMIT GUARD READY"
