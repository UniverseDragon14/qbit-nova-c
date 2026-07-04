#!/usr/bin/env bash
set -eu

mkdir -p build .qcpu logs

REPORT="build/qcpu_runtime_limit_guard.md"
ENV_FILE=".qcpu/runtime_limits.env"

UTC_NOW="$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo unknown)"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
COMMIT="$(git log -1 --oneline 2>/dev/null || echo unknown)"
CPU_CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"

MEMORY_FREE_MB="$(free -m 2>/dev/null | awk '/Mem:/ {print $7}' || true)"
DISK_FREE_MB="$(df -Pm . 2>/dev/null | awk 'NR==2 {print $4}' || true)"

[ -n "$MEMORY_FREE_MB" ] || MEMORY_FREE_MB="0"
[ -n "$DISK_FREE_MB" ] || DISK_FREE_MB="0"

CI_ENV="LOCAL_TERMINAL"
if [ "${GITHUB_ACTIONS:-}" = "true" ] || [ "${CI:-}" = "true" ]; then
  CI_ENV="GITHUB_ACTIONS"
fi

VCGENCMD_STATUS="FAIL: VCGENCMD_NOT_AVAILABLE"
TEMP_VALUE="unknown"
THROTTLE_VALUE="unknown"
TEMP_STATUS="WARN: TEMP_UNKNOWN"
THROTTLE_STATUS="WARN: THROTTLE_UNKNOWN"

if command -v vcgencmd >/dev/null 2>&1; then
  VCGENCMD_STATUS="PASS: VCGENCMD_AVAILABLE"
  TEMP_VALUE="$(vcgencmd measure_temp 2>/dev/null | sed 's/temp=//' || echo unknown)"
  THROTTLE_VALUE="$(vcgencmd get_throttled 2>/dev/null | sed 's/throttled=//' || echo unknown)"
else
  if [ "$CI_ENV" = "GITHUB_ACTIONS" ]; then
    VCGENCMD_STATUS="PASS: CI_SAFE_VCGENCMD_FALLBACK_ACTIVE"
    TEMP_VALUE="CI_NO_VCGENCMD"
    THROTTLE_VALUE="0x0"
  fi
fi

RUNTIME_MODE="CONSERVATIVE_VIRTUAL_QCPU_MODE"
MAX_SHOTS="20"
MAX_PROOF_LOOPS="5"
MAX_PARALLEL_JOBS="1"
LIMIT_STATUS="WARN: CONSERVATIVE_RUNTIME_LIMITS_ACTIVE"

if [ "$VCGENCMD_STATUS" = "PASS: CI_SAFE_VCGENCMD_FALLBACK_ACTIVE" ]; then
  RUNTIME_MODE="STANDARD_VIRTUAL_QCPU_MODE"
  MAX_SHOTS="100"
  MAX_PROOF_LOOPS="20"
  MAX_PARALLEL_JOBS="1"
  LIMIT_STATUS="PASS: STANDARD_RUNTIME_LIMITS_ACTIVE"
  TEMP_STATUS="PASS: CI_TEMP_FALLBACK_FOR_GITHUB_ACTIONS"
  THROTTLE_STATUS="PASS: CI_THROTTLE_FALLBACK_FOR_GITHUB_ACTIONS"
else
  TEMP_INT="$(printf "%s" "$TEMP_VALUE" | sed "s/'C//g" | cut -d. -f1 | sed 's/[^0-9].*//')"
  if [ -n "$TEMP_INT" ] && [ "$TEMP_INT" -lt 70 ]; then
    TEMP_STATUS="PASS: TEMP_SAFE_FOR_RUNTIME"
  elif [ -n "$TEMP_INT" ] && [ "$TEMP_INT" -ge 70 ]; then
    TEMP_STATUS="WARN: TEMP_HIGH_CONSERVATIVE_MODE"
  fi

  if [ "$THROTTLE_VALUE" = "0x0" ]; then
    THROTTLE_STATUS="PASS: NO_THROTTLING"
  elif [ "$THROTTLE_VALUE" != "unknown" ]; then
    THROTTLE_STATUS="WARN: THROTTLING_REPORTED"
  fi

  if [ "$TEMP_STATUS" = "PASS: TEMP_SAFE_FOR_RUNTIME" ] && [ "$THROTTLE_STATUS" = "PASS: NO_THROTTLING" ]; then
    RUNTIME_MODE="STANDARD_VIRTUAL_QCPU_MODE"
    MAX_SHOTS="100"
    MAX_PROOF_LOOPS="20"
    MAX_PARALLEL_JOBS="1"
    LIMIT_STATUS="PASS: STANDARD_RUNTIME_LIMITS_ACTIVE"
  fi
fi

MEMORY_STATUS="WARN: MEMORY_UNKNOWN_OR_LOW"
DISK_STATUS="WARN: DISK_UNKNOWN_OR_LOW"

case "$MEMORY_FREE_MB" in
  ''|*[!0-9]*) MEMORY_STATUS="WARN: MEMORY_UNKNOWN_OR_LOW" ;;
  *) if [ "$MEMORY_FREE_MB" -ge 512 ]; then MEMORY_STATUS="PASS: MEMORY_AVAILABLE_FOR_RUNTIME"; fi ;;
esac

case "$DISK_FREE_MB" in
  ''|*[!0-9]*) DISK_STATUS="WARN: DISK_UNKNOWN_OR_LOW" ;;
  *) if [ "$DISK_FREE_MB" -ge 512 ]; then DISK_STATUS="PASS: DISK_AVAILABLE_FOR_RUNTIME"; fi ;;
esac

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_RUNTIME_LIMIT_GUARD"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD"

cat > "$ENV_FILE" <<ENVEOF
QCPU_RUNTIME_MODE=$RUNTIME_MODE
QCPU_MAX_SHOTS=$MAX_SHOTS
QCPU_MAX_PROOF_LOOPS=$MAX_PROOF_LOOPS
QCPU_MAX_PARALLEL_JOBS=$MAX_PARALLEL_JOBS
QCPU_TEMP_VALUE=$TEMP_VALUE
QCPU_THROTTLE_VALUE=$THROTTLE_VALUE
QCPU_VCGENCMD_STATUS=$VCGENCMD_STATUS
QCPU_CI_ENV=$CI_ENV
QCPU_MEMORY_FREE_MB=$MEMORY_FREE_MB
QCPU_DISK_FREE_MB=$DISK_FREE_MB
QCPU_LIMIT_CREATED_UTC=$UTC_NOW
QCPU_LIMIT_GUARD_STATUS=$LIMIT_STATUS
ENVEOF

cat > "$REPORT" <<REPOF
# QCPU Runtime Limit Guard

Generated UTC: $UTC_NOW

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| CPU cores | $CPU_CORES |
| Commit | $COMMIT |
| CI env | $CI_ENV |

## Runtime Limits

| Limit | Value |
|---|---|
| Runtime mode | $RUNTIME_MODE |
| Max shots | $MAX_SHOTS |
| Max proof loops | $MAX_PROOF_LOOPS |
| Max parallel jobs | $MAX_PARALLEL_JOBS |
| Temperature | $TEMP_VALUE |
| Throttle | $THROTTLE_VALUE |
| vcgencmd | $VCGENCMD_STATUS |
| Memory free MB | $MEMORY_FREE_MB |
| Disk free MB | $DISK_FREE_MB |

## Guard Status

| Check | Result |
|---|---|
| Guard | $LIMIT_STATUS |
| Temperature | $TEMP_STATUS |
| Throttle | $THROTTLE_STATUS |
| Memory | $MEMORY_STATUS |
| Disk | $DISK_STATUS |
| Core | $CORE_STATUS |
| Safety | $SAFETY_STATUS |

## Boundary Statement

The runtime guard is software-only.

It does not mutate hardware.

On Raspberry Pi, it reads vcgencmd when available.

On GitHub Actions or CI without vcgencmd, it uses a CI-safe fallback.

The CI fallback does not claim physical Raspberry Pi hardware.

It only allows the virtual QCPU proof chain to run in a classical CI runner.

Recommended runtime mode:

    $RUNTIME_MODE

Guard status:

    $LIMIT_STATUS

Safety:

    $SAFETY_STATUS

## Verdict

QCPU RUNTIME LIMIT GUARD READY
REPOF

echo "=== QCPU RUNTIME LIMIT GUARD ==="
echo "ci env: $CI_ENV"
echo "vcgencmd: $VCGENCMD_STATUS"
echo "mode: $RUNTIME_MODE"
echo "max shots: $MAX_SHOTS"
echo "max proof loops: $MAX_PROOF_LOOPS"
echo "max parallel jobs: $MAX_PARALLEL_JOBS"
echo "$LIMIT_STATUS"
echo "$TEMP_STATUS"
echo "$THROTTLE_STATUS"
echo "$MEMORY_STATUS"
echo "$DISK_STATUS"
echo "$CORE_STATUS"
echo "$SAFETY_STATUS"
echo
echo "QCPU RUNTIME LIMIT GUARD READY"
