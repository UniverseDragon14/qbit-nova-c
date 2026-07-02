#!/usr/bin/env bash
set -e

echo "=== QBIT NOVA C v2.0 QUICK DEMO ==="

echo
echo "=== BUILD qnova ==="
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
echo "=== BUILD qnova-qasm ==="
gcc src/tools/qasm_export.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    -o qnova-qasm -lm

echo
echo "=== DEMO 1: QMSG virtual qbit register ==="
./qnova examples/qmsg_register.qn | grep -E "QMSG|q0=|q8=|decoded"

echo
echo "=== DEMO 2: Bell-state correlation proof ==="
./scripts/proof_bell.sh 10 | grep -E "BELL STATE|BELL PROOF|Only \|00>|bad count|\|00> count|\|11> count"

echo
echo "=== DEMO 3: OpenQASM export ==="
./qnova-qasm examples/bell_qasm.qn | grep -E "OPENQASM|include|qubit\[2\]|bit\[2\]|h q\[0\]|cx q\[0\], q\[1\]|measure"

echo
echo "QBIT NOVA C v2.0 demo passed"
