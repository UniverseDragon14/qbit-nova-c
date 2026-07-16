#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"

mkdir -p build

gcc \
  -std=c11 \
  -Wall \
  -Wextra \
  -Wpedantic \
  -Werror \
  -O2 \
  -Isrc \
  src/test_qcpu_device_protocol.c \
  -o build/test-qcpu-device-protocol

OUTPUT="$(
  ./build/test-qcpu-device-protocol
)"

printf '%s\n' "$OUTPUT"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "PASS: QCPU_DEVICE_REQUEST_LAYOUT_READY"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "PASS: QCPU_DEVICE_RESPONSE_LAYOUT_READY"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "PASS: QCPU_DEVICE_TRUTH_FLAGS_READY"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "PASS: QCPU_VIRTUAL_DEVICE_PROTOCOL_READY"

echo "PASS: QCPU_DEVICE_PROTOCOL_CONTRACT_READY"
