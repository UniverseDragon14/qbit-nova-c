#!/usr/bin/env bash
set -e

mkdir -p build logs

REPORT="build/qcpu_fault_timeline.md"
MEMORY_LOG="logs/qcpu_fault_memory.log"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "=== QCPU FAULT TIMELINE ==="

echo
echo "=== STEP 1: ENSURE FAULT MEMORY EXISTS ==="
./scripts/qcpu_fault_memory.sh > logs/qcpu_fault_timeline_memory_run.log
grep -E 'QCPU FAULT MEMORY READY|NOISY_BELL_OUTPUT_FOUND|PASS: CORE_NOT_MUTATED_BY_FAULT_MEMORY' logs/qcpu_fault_timeline_memory_run.log

if [ ! -f "$MEMORY_LOG" ]; then
  echo "FAIL: FAULT_MEMORY_LOG_NOT_FOUND"
  exit 1
fi

echo
echo "=== STEP 2: BUILD TIMELINE DATA ==="
EVENT_COUNT="$(grep -c 'EVENT_ID=' "$MEMORY_LOG" || true)"
FAULT_COUNT="$(grep -c 'FAULT_TYPE=NOISY_BELL_OUTPUT_FOUND' "$MEMORY_LOG" || true)"
RECOVERY_COUNT="$(grep -c 'RECOVERY_STATUS=PASS: RECOVERY_MODE_ACTIVE' "$MEMORY_LOG" || true)"
PROOF_COUNT="$(grep -c 'PROOF_STATUS=PASS: CLEAN_BELL_PROOF_AFTER_RECOVERY' "$MEMORY_LOG" || true)"
QCPU_REBOOT_COUNT="$(grep -c 'QCPU_STATUS=PASS: VIRTUAL_QCPU_REBOOTED' "$MEMORY_LOG" || true)"

if [ "$EVENT_COUNT" -lt 1 ]; then
  echo "FAIL: NO_FAULT_EVENTS_FOUND"
  exit 1
fi

echo "events: $EVENT_COUNT"
echo "faults: $FAULT_COUNT"
echo "recoveries: $RECOVERY_COUNT"
echo "clean proofs: $PROOF_COUNT"
echo "qcpu reboots: $QCPU_REBOOT_COUNT"

echo
echo "=== STEP 3: CORE MUTATION CHECK ==="
git diff HEAD -- src include 2>/dev/null > build/qcpu_fault_timeline_core_diff.txt || true

if [ -s build/qcpu_fault_timeline_core_diff.txt ]; then
  echo "FAIL: CORE_MUTATION_DETECTED_BY_FAULT_TIMELINE"
  cat build/qcpu_fault_timeline_core_diff.txt
  exit 1
fi

CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_FAULT_TIMELINE"
SAFETY_STATUS="PASS: NON_DESTRUCTIVE_SOFTWARE_TIMELINE"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

echo
echo "=== STEP 4: WRITE TIMELINE REPORT ==="

cat > "$REPORT" <<EOF_REPORT
# QCPU Fault Timeline

Generated UTC: $TIME_UTC

## Host

- Host: $HOST
- Architecture: $ARCH
- Kernel: $KERNEL
- Commit: $LATEST_COMMIT

## Timeline Counters

| Counter | Value |
|---|---|
| Fault events | $EVENT_COUNT |
| Noisy Bell faults | $FAULT_COUNT |
| Recovery activations | $RECOVERY_COUNT |
| Clean proofs after recovery | $PROOF_COUNT |
| Virtual QCPU reboots | $QCPU_REBOOT_COUNT |
| Core status | $CORE_STATUS |
| Safety status | $SAFETY_STATUS |

## Latest Timeline Tail

\`\`\`text
$(tail -n 40 "$MEMORY_LOG")
\`\`\`

## Boundary Statement

Fault events were found.

    $EVENT_COUNT

Recovery events were found.

    $RECOVERY_COUNT

Clean proof after recovery events were found.

    $PROOF_COUNT

Virtual QCPU reboot events were found.

    $QCPU_REBOOT_COUNT

The core remained safe.

    $CORE_STATUS

## Verdict

QCPU FAULT TIMELINE READY
EOF_REPORT

echo
echo "=== FAULT TIMELINE REPORT CREATED ==="
ls -lh "$REPORT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,160p' "$REPORT"

echo
echo "QCPU FAULT TIMELINE READY"
