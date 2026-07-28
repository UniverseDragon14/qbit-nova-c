#!/usr/bin/env bash
set -e

echo "=== BUILD QBIT NOVA ==="
gcc src/qnova.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    src/vm/byte_vm.c \
    src/vm/state2_vm.c \
    src/quantum/state2.c \
    src/adapter/safe_adapter.c \
    -o qnova -lm

echo
echo "=== TEST full_test.qn ==="
./qnova examples/full_test.qn | grep -E "REPEAT|Loop!"

echo
echo "=== TEST bell_state2.qn ==="
./qnova examples/bell_state2.qn | grep -E "STATE2|CNOT|MEASURE"

echo
echo "=== TEST qmsg_hi.qn ==="
./qnova examples/qmsg_hi.qn | grep -E "QMSG|bits|decoded"

echo
echo "=== TEST qmsg_register.qn ==="
./qnova examples/qmsg_register.qn | grep -E "QMSG|q0=|q8=|decoded"

echo
echo "=== BUILD QASM EXPORTER ==="
gcc src/tools/qasm_export.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    -o qnova-qasm -lm

echo
echo "=== TEST OpenQASM export ==="
./qnova-qasm examples/bell_qasm.qn > /tmp/qbit_nova_bell.qasm
grep -E 'OPENQASM 3.0|include "stdgates.inc"|qubit\[2\] q|bit\[2\] c|h q\[0\]|cx q\[0\], q\[1\]|c\[0\] = measure q\[0\]|c\[1\] = measure q\[1\]' /tmp/qbit_nova_bell.qasm

echo
echo "=== TEST BELL PROOF SCRIPT ==="
./scripts/proof_bell.sh 10

echo
echo "=== TEST QASM FILE EXPORT ==="
./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm
grep -E 'OPENQASM 3.0|include "stdgates.inc"|qubit\[2\] q|bit\[2\] c|h q\[0\]|cx q\[0\], q\[1\]|c\[0\] = measure q\[0\]|c\[1\] = measure q\[1\]' build/bell.qasm

echo
echo "=== TEST VIRTUAL QCPU BOOT ==="
./scripts/qcpu_boot.sh
test -f .qcpu/session.env
test -f build/qcpu_node.json
test -f build/bell.qasm
grep -E 'QBIT_NOVA_VIRTUAL_QCPU|software virtual QCPU' build/qcpu_node.json

echo
echo "=== TEST NOVA HYPERCUBE RUNTIME ==="
./scripts/hypercube_status.sh
grep -E 'NOVA HYPERCUBE RUNTIME READY' <(./scripts/hypercube_status.sh)

echo
echo "ALL QBIT NOVA TESTS PASSED"
