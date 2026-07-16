#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"

mkdir -p build logs .qcpu

CC="${CC:-gcc}"

CFLAGS=(
  -std=c11
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -O2
  -Isrc
)

echo "=== QCPU EXPANSION-COLLAPSE PROOF ==="
echo "boundary: software virtual QCPU"
echo "physical quantum hardware: not claimed"

"$CC" \
  "${CFLAGS[@]}" \
  src/quantum/qcpu_kernel.c \
  src/test_qcpu_kernel.c \
  -lm \
  -o build/test-qcpu-kernel

echo "PASS: QCPU_KERNEL_TEST_BUILD_READY"

./build/test-qcpu-kernel |
tee logs/qcpu_expansion_collapse_test.log

grep -Fq \
  "QCPU EXPANSION-COLLAPSE KERNEL TEST PASSED" \
  logs/qcpu_expansion_collapse_test.log

./scripts/test_qnova_kernel_cli_contract.sh |
tee logs/qcpu_expansion_collapse_cli.log

grep -Fq \
  "PASS: QNOVA_KERNEL_CLI_CONTRACT_READY" \
  logs/qcpu_expansion_collapse_cli.log

grep -Fq \
  "[EXPAND] basis_states=8 norm=1.000000000000" \
  logs/qcpu_expansion_collapse_cli.log

grep -Eq \
  '^\[SHOTS\] total=20 \|000>=[0-9]+ \|111>=[0-9]+ bad=0$' \
  logs/qcpu_expansion_collapse_cli.log

grep -Eq \
  '^\[COLLAPSE\] measured=\|(000|111)> output_bits=3$' \
  logs/qcpu_expansion_collapse_cli.log

CREATED_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

cat > .qcpu/expansion_collapse_kernel.env <<ENV
QCPU_KERNEL_VERSION=v4.5
QCPU_KERNEL_MODE=STANDARD_VIRTUAL_QCPU_MODE
QCPU_KERNEL_QUBITS=3
QCPU_KERNEL_STATE_COUNT=8
QCPU_KERNEL_SHOTS=20
QCPU_KERNEL_BAD_RESULTS=0
QCPU_KERNEL_STATUS=PASS: QCPU_EXPANSION_COLLAPSE_KERNEL_READY
QCPU_KERNEL_CREATED_UTC=$CREATED_UTC
ENV

cat > build/qcpu_expansion_collapse_kernel.md <<REPORT
# QCPU Expansion-Collapse Kernel

Generated UTC: $CREATED_UTC

## Verified Runtime

| Field | Result |
|---|---|
| Version | v4.5 |
| Mode | STANDARD_VIRTUAL_QCPU_MODE |
| Compact input | 3-qubit GHZ circuit |
| Expanded state count | 8 basis states |
| Normalization | PASS |
| Shot count | 20 |
| Invalid GHZ outcomes | 0 |
| Collapse result | 000 or 111 |

## State

The compact circuit prepares:

    |000> = 0.707107
    |111> = 0.707107

All other basis amplitudes are zero within the tested tolerance.

## Boundary

This is a C software statevector Virtual QCPU.

It does not claim physical quantum hardware.

## Verdict

PASS: QCPU_EXPANSION_COLLAPSE_KERNEL_READY
REPORT

echo "PASS: QCPU_EXPANSION_COLLAPSE_REPORT_READY"
echo "PASS: QCPU_EXPANSION_COLLAPSE_KERNEL_READY"
echo "QCPU EXPANSION-COLLAPSE KERNEL READY"
