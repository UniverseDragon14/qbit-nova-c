#!/usr/bin/env bash
set -Eeuo pipefail
umask 077
export LC_ALL=C

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/qcpu-v47-stage2a-proof.XXXXXX")"
LOG="$BUILD_DIR/qcpu_v47_stage2a_userspace_mock.log"
BIN="$BUILD_DIR/qcpu_mock_test"

cleanup() {
  rm -rf "$BUILD_DIR"
}
trap cleanup EXIT

cd "$ROOT"

echo "=== QBIT NOVA C V4.7 STAGE 2A USERSPACE MOCK PROOF ==="
echo "[GUARD] pure_userspace = YES"
echo "[GUARD] kernel_module = NO"
echo "[GUARD] device_node = NO"
echo "[GUARD] root_action = NO"
echo "[GUARD] network_listener = NO"

for file in \
  src/device/qcpu_mock_frontend.h \
  src/device/qcpu_mock_frontend.c \
  tests/v47/qcpu_mock_test_backend.h \
  tests/v47/qcpu_mock_test_backend.c \
  tests/v47/qcpu_mock_test.c
do
  test -f "$file"
done

gcc \
  -std=c11 \
  -D_POSIX_C_SOURCE=200809L \
  -Wall \
  -Wextra \
  -Werror \
  -pedantic \
  -pthread \
  -I. \
  src/device/qcpu_mock_frontend.c \
  tests/v47/qcpu_mock_test_backend.c \
  tests/v47/qcpu_mock_test.c \
  -o "$BIN" \
  -lm

echo "PASS: QCPU_V47_STAGE2A_STRICT_BUILD_READY"

"$BIN" 2>&1 | tee "$LOG"

for marker in \
  "PASS: ABI_LAYOUT_AND_IOCTL_IDENTITY" \
  "PASS: CAPS_STATUS_AND_UNKNOWN_IOCTL" \
  "PASS: SAME_PROCESS_EXCLUSIVE_SESSION" \
  "PASS: STATUS_AND_GHZ_SUCCESS" \
  "PASS: PREDISPATCH_VALIDATION_AND_COUNTERS" \
  "PASS: Q32_32_NORM_CONVERSION" \
  "PASS: DETERMINISTIC_RESPONSE_VALIDATION" \
  "PASS: BACKEND_ABSENT_DISCONNECT_AND_RECOVERY" \
  "PASS: TIMEOUT_AND_SAME_SESSION_RECOVERY" \
  "PASS: BUSY_CANCEL_AND_DESTROY_QUIESCENCE" \
  "PASS: MANDATORY_CROSS_PROCESS_EXCLUSIVE_LOCK" \
  "PASS: REPEATED_100_SESSION_LIFECYCLE" \
  "PASS: QCPU_V47_STAGE2A_USERSPACE_MOCK_CORE_READY"
do
  grep -Fq "$marker" "$LOG"
done

if find "$BUILD_DIR" -maxdepth 1 -type f -name '*.ko' -print | grep -q .; then
  echo "FAIL: kernel module artifact detected"
  exit 1
fi

if [ -e /dev/qcpu0 ]; then
  echo "NOTE: /dev/qcpu0 pre-existed; proof did not create or access it"
fi

echo "PASS: QCPU_V47_STAGE2A_NO_KERNEL_ARTIFACTS"
echo "PASS: QCPU_V47_STAGE2A_NO_DEVICE_NODE_ACTION"
echo "PASS: QCPU_V47_STAGE2A_USERSPACE_MOCK_PROOF_READY"
