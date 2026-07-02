#!/usr/bin/env bash
set -e

echo "=== NOVA HYPERCUBE RUNTIME STATUS ==="

echo
echo "=== NODE CHECK ==="
if [ ! -f build/qcpu_node.json ]; then
    echo "qcpu_node.json not found. Booting virtual QCPU first..."
    ./scripts/qcpu_boot.sh
fi

echo
echo "=== NODE IDENTITY ==="
grep -E '"node"|"mode"|"host"|"arch"|"runtime"|"safety_boundary"' build/qcpu_node.json

echo
echo "=== SESSION CHECK ==="
test -f .qcpu/session.env
grep -E 'QCPU_MODE|QCPU_RUNTIME|QCPU_BOUNDARY' .qcpu/session.env

echo
echo "=== QASM CHECK ==="
test -f build/bell.qasm
grep -E 'OPENQASM 3.0|qubit\[2\] q|h q\[0\]|cx q\[0\], q\[1\]|measure' build/bell.qasm

echo
echo "NOVA HYPERCUBE RUNTIME READY"
