#!/usr/bin/env bash
set -Eeuo pipefail
umask 077
export LC_ALL=C

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/novakutty-build-week.XXXXXX")"
ADMISSION_LOG="$TMP_ROOT/admission.log"
ADAPTER_LOG="$TMP_ROOT/adapter.log"
RECEIPT="$ROOT/build/novakutty_build_week_receipt.md"

cleanup() {
  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT

cd "$ROOT"
mkdir -p build

echo "============================================================"
echo "NOVAKUTTY - OPENAI BUILD WEEK JUDGE DEMO"
echo "============================================================"
echo "CREATOR_OWNER=UNIVERSAL_DRAGON_ASLAM"
echo "PRODUCT_BRAND=NOVAKUTTY"
echo "CORE_TECHNOLOGY=QBIT_NOVA_C"
echo "USER_FACING_ASSISTANT=NOVA_EVE"
echo "PRODUCT_TYPE=APPROVAL_FIRST_SOFTWARE_VIRTUAL_QCPU_RUNTIME"
echo "HOST=CLASSICAL_CPU"
echo "PHYSICAL_QPU_PRESENT=NO"
echo

echo "[1/3] APPROVAL GATE"
bash scripts/qcpu_workload_admission.sh >"$ADMISSION_LOG"

grep -Fq "QCPU_STANDARD_ADMISSION=ADMIT_WORKLOAD" \
  .qcpu/workload_admission.env
grep -Fq "QCPU_HEAVY_ADMISSION=REJECT_WORKLOAD" \
  .qcpu/workload_admission.env
grep -Fq "PASS: WORKLOAD_ADMISSION_POLICY_ENFORCED" \
  .qcpu/workload_admission.env

echo "SAFE_STANDARD_REQUEST=APPROVED"
echo "UNSAFE_HEAVY_REQUEST=REJECTED"
echo "PASS: BUILD_WEEK_APPROVAL_GATE_READY"
echo

echo "[2/3] VERIFIED USERSPACE QCPUD BRIDGE"
bash scripts/proof_qcpu_v47_qcpud_adapter.sh | tee "$ADAPTER_LOG"

grep -Fq "PASS: QCPU_V47_STAGE2B_STATUS_BRIDGE_READY" \
  "$ADAPTER_LOG"
grep -Fq "PASS: QCPU_V47_STAGE2B_GHZ_BRIDGE_READY" \
  "$ADAPTER_LOG"
grep -Fq "PASS: QCPU_V47_STAGE2B_BOUNDED_STALL_READY" \
  "$ADAPTER_LOG"
grep -Fq "PASS: QCPU_V47_STAGE2B_QCPUD_ADAPTER_PROOF_READY" \
  "$ADAPTER_LOG"

echo
echo "[3/3] EVIDENCE RECEIPT"
cat >"$RECEIPT" <<EOF
# Novakutty Build Week Demo Receipt

Generated UTC: $(date -u +%Y-%m-%dT%H:%M:%SZ)

Creator and owner: Universal Dragon Aslam

Core technology: QBIT NOVA C

User-facing assistant identity: NOVA / EVE

## Product result

- Safe standard request: APPROVED
- Unsafe heavy request: REJECTED
- Stage 2 STATUS bridge: PASS
- Stage 2 RUN_GHZ bridge: PASS
- Q32.32 conversion boundary: PASS
- Bounded backend stall: PASS
- Graceful qcpud cleanup: PASS

## Truth boundary

- Software Virtual QCPU only
- Classical host
- Physical QPU absent
- No kernel module
- No device-node creation
- No root action
- No TCP or UDP listener

## Verdict

PASS: NOVAKUTTY_BUILD_WEEK_DEMO_READY
EOF

echo "RECEIPT=build/novakutty_build_week_receipt.md"
echo "PASS: NOVAKUTTY_BUILD_WEEK_DEMO_READY"
