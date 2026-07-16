#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"

CLI_SOURCE="src/tools/qnova_kernel_cli.c"
KERNEL_SOURCE="src/quantum/qcpu_kernel.c"
BINARY="build/qnova-kernel"

if [ ! -f "$CLI_SOURCE" ]; then
  echo "RED: QNOVA_KERNEL_CLI_SOURCE_MISSING"
  exit 20
fi

mkdir -p build

gcc \
  -std=c11 \
  -Wall \
  -Wextra \
  -Wpedantic \
  -Werror \
  -O2 \
  -Isrc \
  "$KERNEL_SOURCE" \
  "$CLI_SOURCE" \
  -lm \
  -o "$BINARY"

OUTPUT="$(
  "$BINARY" \
    3 \
    20 \
    424242
)"

printf '%s\n' "$OUTPUT"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "[INPUT] compact_ghz_ops=3 qubits=3"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "[EXPAND] basis_states=8 norm=1.000000000000"

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "[STATE] |000>=0.707107 |111>=0.707107"

printf '%s\n' "$OUTPUT" |
grep -Eq \
  '^\[SHOTS\] total=20 \|000>=[0-9]+ \|111>=[0-9]+ bad=0$'

printf '%s\n' "$OUTPUT" |
grep -Eq \
  '^\[COLLAPSE\] measured=\|(000|111)> output_bits=3$'

printf '%s\n' "$OUTPUT" |
grep -Fq \
  "PASS: QCPU_EXPANSION_COLLAPSE_KERNEL_READY"

echo "PASS: QNOVA_KERNEL_CLI_CONTRACT_READY"
