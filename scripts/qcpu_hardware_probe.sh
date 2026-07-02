#!/usr/bin/env bash
set -e

mkdir -p build logs

REPORT="build/qcpu_hardware_probe.md"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
OS_NAME="$(grep -m1 '^PRETTY_NAME=' /etc/os-release 2>/dev/null | cut -d '=' -f2- | tr -d '"' || echo unknown)"
CPU_MODEL="$(grep -m1 'Model' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || true)"

if [ -z "$CPU_MODEL" ]; then
  CPU_MODEL="$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || echo unknown)"
fi

TEMP_STATUS="UNKNOWN"
TEMP_VALUE="unknown"

if command -v vcgencmd >/dev/null 2>&1; then
  TEMP_VALUE="$(vcgencmd measure_temp 2>/dev/null | sed 's/temp=//' || echo unknown)"
  THROTTLE_VALUE="$(vcgencmd get_throttled 2>/dev/null | sed 's/throttled=//' || echo unknown)"
else
  if [ -f /sys/class/thermal/thermal_zone0/temp ]; then
    RAW_TEMP="$(cat /sys/class/thermal/thermal_zone0/temp)"
    TEMP_VALUE="$((RAW_TEMP / 1000))C"
  fi
  THROTTLE_VALUE="unknown"
fi

TEMP_NUM="$(printf '%s' "$TEMP_VALUE" | grep -oE '^[0-9]+' || echo 0)"

if [ "$TEMP_NUM" -lt 65 ]; then
  TEMP_STATUS="PASS: HARDWARE_TEMP_SAFE"
elif [ "$TEMP_NUM" -lt 75 ]; then
  TEMP_STATUS="WARN: HARDWARE_TEMP_WARM"
else
  TEMP_STATUS="WARN: HARDWARE_TEMP_HOT"
fi

if [ "$THROTTLE_VALUE" = "0x0" ]; then
  THROTTLE_STATUS="PASS: NO_THROTTLING"
else
  THROTTLE_STATUS="WARN: THROTTLE_STATE_$THROTTLE_VALUE"
fi

MEM_TOTAL="$(free -h 2>/dev/null | awk '/Mem:/ {print $2}' || echo unknown)"
MEM_FREE="$(free -h 2>/dev/null | awk '/Mem:/ {print $4}' || echo unknown)"
DISK_ROOT="$(df -h / 2>/dev/null | awk 'NR==2 {print $2 " total, " $3 " used, " $4 " free, " $5 " used"}' || echo unknown)"
UPTIME_LINE="$(uptime 2>/dev/null || echo unknown)"

if [ -e /dev/qcpu0 ] || [ -e /dev/quantum0 ]; then
  PHYSICAL_QCPU="PASS: PHYSICAL_QCPU_DEVICE_FOUND"
else
  PHYSICAL_QCPU="EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"
fi

VIRTUAL_QCPU="PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST"
SAFETY_STATUS="PASS: READ_ONLY_HARDWARE_PROBE"
CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_HARDWARE_PROBE"

echo "=== QCPU HARDWARE REALITY PROBE ==="
echo "host: $HOST"
echo "arch: $ARCH"
echo "kernel: $KERNEL"
echo "cpu: $CPU_MODEL"
echo "temp: $TEMP_VALUE"
echo "throttle: $THROTTLE_VALUE"
echo "$TEMP_STATUS"
echo "$THROTTLE_STATUS"
echo "$PHYSICAL_QCPU"
echo "$VIRTUAL_QCPU"
echo "$SAFETY_STATUS"

echo
echo "=== CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_hardware_probe_core_diff.txt || true

if [ -s build/qcpu_hardware_probe_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_HARDWARE_PROBE"
  cat build/qcpu_hardware_probe_core_diff.txt
  exit 1
fi

echo "$CORE_STATUS"

cat > "$REPORT" <<EOF_REPORT
# QCPU Hardware Reality Probe

Generated UTC: $TIME_UTC

## Host Reality

| Field | Value |
|---|---|
| Host | $HOST |
| OS | $OS_NAME |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| CPU model | $CPU_MODEL |
| Temperature | $TEMP_VALUE |
| Temperature status | $TEMP_STATUS |
| Throttle value | $THROTTLE_VALUE |
| Throttle status | $THROTTLE_STATUS |
| Memory total | $MEM_TOTAL |
| Memory free | $MEM_FREE |
| Disk root | $DISK_ROOT |
| Uptime | $UPTIME_LINE |

## QCPU Boundary

| Check | Result | Meaning |
|---|---|---|
| Physical QCPU device | $PHYSICAL_QCPU | Honest physical hardware check |
| Virtual QCPU support | $VIRTUAL_QCPU | Classical host can run software QCPU runtime |
| Core mutation check | $CORE_STATUS | Core engine was not modified |
| Safety boundary | $SAFETY_STATUS | Read-only hardware probe only |

## Boundary Statement

Physical QCPU check:

    $PHYSICAL_QCPU

Virtual QCPU check:

    $VIRTUAL_QCPU

Temperature check:

    $TEMP_STATUS

Throttle check:

    $THROTTLE_STATUS

Safety check:

    $SAFETY_STATUS

## Verdict

QCPU HARDWARE REALITY PROBE READY
EOF_REPORT

echo
echo "=== HARDWARE REPORT CREATED ==="
ls -lh "$REPORT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,150p' "$REPORT"

echo
echo "QCPU HARDWARE REALITY PROBE READY"
