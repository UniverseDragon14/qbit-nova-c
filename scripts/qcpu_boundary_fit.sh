#!/usr/bin/env bash
set -e

mkdir -p build logs

REPORT="build/qcpu_boundary_fit.md"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
CPU_MODEL="$(grep -m1 'Model' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || true)"

if [ -z "$CPU_MODEL" ]; then
  CPU_MODEL="$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ *//' || echo unknown)"
fi

echo "=== QCPU BOUNDARY FIT MATRIX ==="

echo
echo "=== STONE 1: PHYSICAL QCPU DEVICE CHECK ==="
PHYSICAL_QCPU="EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND"

if [ -e /dev/qcpu0 ] || [ -e /dev/quantum0 ]; then
  PHYSICAL_QCPU="PASS: PHYSICAL_QCPU_DEVICE_FOUND"
fi

echo "$PHYSICAL_QCPU"

echo
echo "=== STONE 2: VIRTUAL QCPU BOOT ==="
./scripts/qcpu_boot.sh > logs/qcpu_boundary_boot.log
grep -E 'QCPU NODE READY|software virtual QCPU|QBIT_NOVA_VIRTUAL_QCPU' logs/qcpu_boundary_boot.log
VIRTUAL_QCPU="PASS: VIRTUAL_QCPU_READY"

echo
echo "=== STONE 3: BELL PROOF ==="
./scripts/proof_bell.sh 10 > logs/qcpu_boundary_bell.log
grep -E 'BELL PROOF PASSED|bad count: 0' logs/qcpu_boundary_bell.log
BELL_PROOF="PASS: BELL_PROOF_READY"

echo
echo "=== STONE 4: QASM BRIDGE ==="
./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm > logs/qcpu_boundary_qasm.log
grep -E 'OPENQASM 3.0|qubit\[2\] q|cx q\[0\], q\[1\]' build/bell.qasm
QASM_BRIDGE="PASS: QASM_BRIDGE_READY"

echo
echo "=== STONE 5: HYPERCUBE STATUS ==="
./scripts/hypercube_status.sh > logs/qcpu_boundary_hypercube.log
grep -E 'NOVA HYPERCUBE RUNTIME READY' logs/qcpu_boundary_hypercube.log
HYPERCUBE_STATUS="PASS: HYPERCUBE_RUNTIME_READY"

echo
echo "=== STONE 6: SNAPSHOT REPORT ==="
./scripts/hypercube_snapshot.sh > logs/qcpu_boundary_snapshot.log
test -f build/hypercube_snapshot.md
grep -E 'NOVA Hypercube Runtime Snapshot|Safety Boundary' build/hypercube_snapshot.md
SNAPSHOT_STATUS="PASS: HYPERCUBE_SNAPSHOT_READY"

cat > "$REPORT" <<EOF
# QCPU Boundary Fit Matrix

Generated UTC: $TIME_UTC

## Host

- Host: $HOST
- Architecture: $ARCH
- Kernel: $KERNEL
- CPU model: $CPU_MODEL

## Boundary Stones

| Stone | Result | Meaning |
|---|---|---|
| Physical QCPU device | $PHYSICAL_QCPU | Honest hardware boundary check |
| Virtual QCPU boot | $VIRTUAL_QCPU | Software runtime is available |
| Bell proof | $BELL_PROOF | Entanglement simulation proof passes |
| QASM bridge | $QASM_BRIDGE | OpenQASM export path works |
| Hypercube status | $HYPERCUBE_STATUS | Runtime identity layer works |
| Snapshot report | $SNAPSHOT_STATUS | Runtime receipt created |

## Final Boundary Statement

The physical quantum hardware stone is allowed to fall.

That is not hidden.

    $PHYSICAL_QCPU

The virtual QCPU stone fits.

    $VIRTUAL_QCPU

## Safety Boundary

This is a non-destructive software boundary probe.

No hardware mutation, voltage control, kernel patching, or destructive system action is performed.

## Verdict

QCPU BOUNDARY FIT MATRIX READY
EOF

echo
echo "=== BOUNDARY REPORT CREATED ==="
ls -lh "$REPORT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,120p' "$REPORT"

echo
echo "QCPU BOUNDARY FIT MATRIX READY"
