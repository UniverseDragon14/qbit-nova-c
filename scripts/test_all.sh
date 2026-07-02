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
./scripts/hypercube_status.sh > /tmp/qbit_nova_hypercube_status.log
cat /tmp/qbit_nova_hypercube_status.log
grep -E 'NOVA HYPERCUBE RUNTIME READY' /tmp/qbit_nova_hypercube_status.log

echo
echo "=== TEST NOVA HYPERCUBE SNAPSHOT ==="
./scripts/hypercube_snapshot.sh
test -f build/hypercube_snapshot.md
grep -E "NOVA Hypercube Runtime Snapshot|Safety Boundary|QASM Preview|Bell Proof Summary" build/hypercube_snapshot.md

echo
echo "=== TEST QCPU BOUNDARY FIT MATRIX ==="
./scripts/qcpu_boundary_fit.sh
test -f build/qcpu_boundary_fit.md
grep -E "QCPU BOUNDARY FIT MATRIX READY|EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND|PASS: VIRTUAL_QCPU_READY" build/qcpu_boundary_fit.md

echo
echo "=== TEST QCPU NOISE INJECTION MATRIX ==="
./scripts/qcpu_noise_injection.sh
test -f build/qcpu_noise_injection.md
grep -E "QCPU NOISE INJECTION MATRIX READY|EXPECTED_DETECT: NOISY_BELL_OUTPUT_FOUND|PASS: CLEAN_BELL_PROOF_READY" build/qcpu_noise_injection.md

echo
echo "=== TEST QCPU RECOVERY MATRIX ==="
./scripts/qcpu_recovery_matrix.sh
test -f build/qcpu_recovery_matrix.md
grep -E "QCPU RECOVERY MATRIX READY|PASS: RECOVERY_MODE_ACTIVE|PASS: CLEAN_BELL_PROOF_AFTER_RECOVERY|PASS: VIRTUAL_QCPU_REBOOTED" build/qcpu_recovery_matrix.md

echo
echo "=== TEST QCPU FAULT MEMORY ==="
./scripts/qcpu_fault_memory.sh
test -f build/qcpu_fault_memory.md
test -f logs/qcpu_fault_memory.log
grep -E "QCPU FAULT MEMORY READY|NOISY_BELL_OUTPUT_FOUND|PASS: CORE_NOT_MUTATED_BY_FAULT_MEMORY" build/qcpu_fault_memory.md

echo
echo "=== TEST QCPU FAULT TIMELINE ==="
./scripts/qcpu_fault_timeline.sh
test -f build/qcpu_fault_timeline.md
grep -E "QCPU FAULT TIMELINE READY|PASS: CORE_NOT_MUTATED_BY_FAULT_TIMELINE|Fault events" build/qcpu_fault_timeline.md

echo
echo "ALL QBIT NOVA TESTS PASSED"
