#!/usr/bin/env bash
set -e

mkdir -p build logs

REPORT="build/qcpu_hardware_capability_map.md"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
OS_NAME="$(grep -m1 '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2- | tr -d '"' || echo unknown)"
CPU_MODEL="$(grep -m1 'Model' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || true)"

if [ -z "$CPU_MODEL" ]; then
  CPU_MODEL="$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || echo unknown)"
fi

CPU_CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1)"
MEM_MB="$(awk '/MemTotal:/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 0)"
MEM_FREE_MB="$(awk '/MemAvailable:/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 0)"
DISK_FREE_MB="$(df -Pm / 2>/dev/null | awk 'NR==2 {print $4}' || echo 0)"
DISK_USED_PCT="$(df -P / 2>/dev/null | awk 'NR==2 {print $5}' || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

TEMP_VALUE="unknown"
TEMP_NUM=0
THROTTLE_VALUE="unknown"

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

echo "=== QCPU HARDWARE CAPABILITY MAP ==="

echo
echo "=== STEP 1: RUN HARDWARE REALITY PROBE ==="
./scripts/qcpu_hardware_probe.sh > logs/qcpu_capability_hardware_probe.log
grep -E 'QCPU HARDWARE REALITY PROBE READY|EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND|PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST|PASS: READ_ONLY_HARDWARE_PROBE' logs/qcpu_capability_hardware_probe.log

echo
echo "=== STEP 2: MAP CAPABILITIES ==="

if [ "$CPU_CORES" -ge 4 ]; then
  CPU_STATUS="PASS: CPU_CORE_CAPACITY_OK"
else
  CPU_STATUS="WARN: CPU_CORE_CAPACITY_LOW"
fi

if [ "$MEM_MB" -ge 4096 ]; then
  RAM_STATUS="PASS: RAM_CAPACITY_OK"
else
  RAM_STATUS="WARN: RAM_CAPACITY_LOW"
fi

if [ "$MEM_FREE_MB" -ge 1024 ]; then
  RAM_FREE_STATUS="PASS: RAM_AVAILABLE_OK"
else
  RAM_FREE_STATUS="WARN: RAM_AVAILABLE_LOW"
fi

if [ "$DISK_FREE_MB" -ge 1024 ]; then
  DISK_STATUS="PASS: DISK_CAPACITY_OK"
else
  DISK_STATUS="WARN: DISK_CAPACITY_LOW"
fi

if [ "$TEMP_NUM" -lt 65 ]; then
  TEMP_STATUS="PASS: THERMAL_CAPACITY_OK"
elif [ "$TEMP_NUM" -lt 75 ]; then
  TEMP_STATUS="WARN: THERMAL_CAPACITY_WARM"
else
  TEMP_STATUS="WARN: THERMAL_CAPACITY_HOT"
fi

if [ "$THROTTLE_VALUE" = "0x0" ]; then
  THROTTLE_STATUS="PASS: THROTTLE_CLEAR"
else
  THROTTLE_STATUS="WARN: THROTTLE_STATE_$THROTTLE_VALUE"
fi

TOOL_GCC="WARN: GCC_NOT_FOUND"
TOOL_PYTHON="WARN: PYTHON3_NOT_FOUND"
TOOL_GIT="WARN: GIT_NOT_FOUND"

command -v gcc >/dev/null 2>&1 && TOOL_GCC="PASS: GCC_AVAILABLE"
command -v python3 >/dev/null 2>&1 && TOOL_PYTHON="PASS: PYTHON3_AVAILABLE"
command -v git >/dev/null 2>&1 && TOOL_GIT="PASS: GIT_AVAILABLE"

if [ "$TEMP_NUM" -lt 65 ] && [ "$THROTTLE_VALUE" = "0x0" ] && [ "$MEM_FREE_MB" -ge 1024 ] && [ "$DISK_FREE_MB" -ge 1024 ]; then
  QCPU_MODE="STANDARD_VIRTUAL_QCPU_MODE"
  QCPU_MODE_STATUS="PASS: STANDARD_MODE_RECOMMENDED"
elif [ "$TEMP_NUM" -lt 75 ] && [ "$MEM_FREE_MB" -ge 512 ]; then
  QCPU_MODE="CONSERVATIVE_VIRTUAL_QCPU_MODE"
  QCPU_MODE_STATUS="WARN: CONSERVATIVE_MODE_RECOMMENDED"
else
  QCPU_MODE="REST_REQUIRED_MODE"
  QCPU_MODE_STATUS="WARN: REST_BEFORE_HEAVY_QCPU_WORK"
fi

echo "cpu cores: $CPU_CORES"
echo "memory total MB: $MEM_MB"
echo "memory free MB: $MEM_FREE_MB"
echo "disk free MB: $DISK_FREE_MB"
echo "disk used: $DISK_USED_PCT"
echo "temperature: $TEMP_VALUE"
echo "throttle: $THROTTLE_VALUE"
echo "$CPU_STATUS"
echo "$RAM_STATUS"
echo "$RAM_FREE_STATUS"
echo "$DISK_STATUS"
echo "$TEMP_STATUS"
echo "$THROTTLE_STATUS"
echo "$QCPU_MODE_STATUS"

echo
echo "=== STEP 3: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_hardware_capability_core_diff.txt || true

if [ -s build/qcpu_hardware_capability_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_CAPABILITY_MAP"
  cat build/qcpu_hardware_capability_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_CAPABILITY_MAP"
SAFETY_STATUS="PASS: READ_ONLY_CAPABILITY_MAP"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Hardware Capability Map

Generated UTC: $TIME_UTC

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| OS | $OS_NAME |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| CPU model | $CPU_MODEL |
| Commit | $LATEST_COMMIT |

## Hardware Capability

| Capability | Value | Status |
|---|---|---|
| CPU cores | $CPU_CORES | $CPU_STATUS |
| RAM total | ${MEM_MB} MB | $RAM_STATUS |
| RAM available | ${MEM_FREE_MB} MB | $RAM_FREE_STATUS |
| Disk free | ${DISK_FREE_MB} MB | $DISK_STATUS |
| Disk used | $DISK_USED_PCT | PASS: DISK_USAGE_RECORDED |
| Temperature | $TEMP_VALUE | $TEMP_STATUS |
| Throttle | $THROTTLE_VALUE | $THROTTLE_STATUS |

## Tool Capability

| Tool | Status |
|---|---|
| gcc | $TOOL_GCC |
| python3 | $TOOL_PYTHON |
| git | $TOOL_GIT |

## QCPU Runtime Recommendation

| Field | Value |
|---|---|
| Recommended mode | $QCPU_MODE |
| Mode status | $QCPU_MODE_STATUS |
| Core status | $CORE_STATUS |
| Safety status | $SAFETY_STATUS |

## Hardware Reality Probe Summary

\`\`\`text
$(grep -E 'QCPU HARDWARE REALITY PROBE READY|EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND|PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST|PASS: READ_ONLY_HARDWARE_PROBE' logs/qcpu_capability_hardware_probe.log)
\`\`\`

## Boundary Statement

This capability map is read-only.

It does not convert classical hardware into physical quantum hardware.

Physical QCPU remains:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

Virtual QCPU capability is:

    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST

Recommended runtime mode:

    $QCPU_MODE

Safety:

    $SAFETY_STATUS

## Verdict

QCPU HARDWARE CAPABILITY MAP READY
EOF_REPORT

echo
echo "=== CAPABILITY MAP REPORT CREATED ==="
ls -lh "$REPORT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,170p' "$REPORT"

echo
echo "QCPU HARDWARE CAPABILITY MAP READY"
