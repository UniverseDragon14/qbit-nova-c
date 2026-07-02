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
echo "ALL QBIT NOVA TESTS PASSED"
