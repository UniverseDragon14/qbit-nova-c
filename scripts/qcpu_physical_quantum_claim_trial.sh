#!/usr/bin/env bash
set -euo pipefail

echo "=== QCPU PHYSICAL QUANTUM CLAIM TRIAL (honest) ==="
mkdir -p build .qcpu
REPORT="build/qcpu_physical_quantum_claim_trial.md"
ENVF=".qcpu/physical_quantum_claim_trial.env"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m)"; KERNEL="$(uname -r)"
CPU="$(tr -d '\0' </proc/device-tree/model 2>/dev/null || echo unknown)"
UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "host: $HOST  arch: $ARCH  cpu: $CPU"

# STEP 2: honest physical QPU check.
# A real quantum processor exposes NO standard Linux consumer device.
# NOTE: names like /dev/ion (DMA allocator) are NOT ion-trap quantum HW,
# so we do NOT substring-match /dev. There is no physical QPU here.
PHYS="EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"
echo "physical qpu: $PHYS"

# STEP 3: software virtual QCPU must work
gcc src/qnova.c \
  src/lexer/lexer.c \
  src/parser/parser.c \
  src/compiler/compiler.c \
  src/vm/byte_vm.c \
  src/vm/state2_vm.c \
  src/quantum/state2.c \
  src/adapter/safe_adapter.c \
  -o qnova -lm

if ./qnova examples/bell_state2.qn 2>/dev/null | grep -q "MEASURE pair"; then
  VIRT="PASS: SOFTWARE_VIRTUAL_QCPU_WORKS"
  DECISION="HONEST: PI_IS_NOT_PHYSICAL_QUANTUM_BUT_VIRTUAL_QCPU_VALID"
  FINAL_STATUS="PASS: HONEST_CLAIM_TRIAL_READY"
  EXIT_CODE=0
else
  VIRT="FAIL: SOFTWARE_VIRTUAL_QCPU_NOT_WORKING"
  DECISION="HONEST: PI_IS_NOT_PHYSICAL_QUANTUM_AND_VIRTUAL_QCPU_INVALID"
  FINAL_STATUS="FAIL: HONEST_CLAIM_TRIAL_VIRTUAL_QCPU_BROKEN"
  EXIT_CODE=1
fi

echo "virtual qcpu: $VIRT"
echo "verdict: $DECISION"

cat > "$REPORT" <<MD
# QCPU Physical Quantum Claim Trial

Generated UTC: $UTC

## Claim Tested

"Raspberry Pi became a physical quantum computer."

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| CPU | $CPU |
| Commit | $COMMIT |

## Results

| Check | Result |
|---|---|
| Physical QCPU hardware | $PHYS |
| Software virtual QCPU | $VIRT |
| Final decision | $DECISION |

## Truth Boundary

Raspberry Pi did NOT become physical quantum hardware.
No physical quantum processor device exists on this host.
(Device nodes such as /dev/ion are DMA memory allocators, not ion-trap qubits.)

QBIT NOVA C runs as a C-based software virtual QCPU on classical hardware.
This is real and useful: a quantum-style language, Bell proof chain, and
OpenQASM bridge — not physical qubit hardware.

## Verdict

$DECISION

$FINAL_STATUS
MD

{
  echo "QCPU_CLAIM_PHYSICAL=$PHYS"
  echo "QCPU_CLAIM_VIRTUAL=$VIRT"
  echo "QCPU_CLAIM_DECISION=$DECISION"
  echo "QCPU_CLAIM_STATUS=$FINAL_STATUS"
  echo "QCPU_CLAIM_CREATED_UTC=$UTC"
} > "$ENVF"

echo "report: $REPORT"
echo "$FINAL_STATUS"
exit "$EXIT_CODE"
