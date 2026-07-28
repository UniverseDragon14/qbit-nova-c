#!/usr/bin/env bash
set -e
QBIT_TMP_DIR="${TMPDIR:-${PREFIX:-/data/data/com.termux/files/usr}/tmp}"
mkdir -p "$QBIT_TMP_DIR"
QBIT_QASM_OUT="$QBIT_TMP_DIR/qbit_nova_bell.qasm"

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
./qnova-qasm examples/bell_qasm.qn > $QBIT_QASM_OUT
grep -E 'OPENQASM 3.0|include "stdgates.inc"|qubit\[2\] q|bit\[2\] c|h q\[0\]|cx q\[0\], q\[1\]|c\[0\] = measure q\[0\]|c\[1\] = measure q\[1\]' $QBIT_QASM_OUT

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
./scripts/hypercube_status.sh > $QBIT_TMP_DIR/qbit_nova_hypercube_status.log
cat $QBIT_TMP_DIR/qbit_nova_hypercube_status.log
grep -E 'NOVA HYPERCUBE RUNTIME READY' $QBIT_TMP_DIR/qbit_nova_hypercube_status.log

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
echo "=== TEST QCPU HARDWARE REALITY PROBE ==="
./scripts/qcpu_hardware_probe.sh
test -f build/qcpu_hardware_probe.md
grep -E "QCPU HARDWARE REALITY PROBE READY|EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND|PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST" build/qcpu_hardware_probe.md

echo
echo "=== TEST QCPU HARDWARE CAPABILITY MAP ==="
./scripts/qcpu_hardware_capability_map.sh
test -f build/qcpu_hardware_capability_map.md
grep -E "QCPU HARDWARE CAPABILITY MAP READY|PASS: READ_ONLY_CAPABILITY_MAP|VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST" build/qcpu_hardware_capability_map.md

echo
echo "=== TEST QCPU RUNTIME LIMIT GUARD ==="
./scripts/qcpu_runtime_limit_guard.sh
test -f build/qcpu_runtime_limit_guard.md
test -f .qcpu/runtime_limits.env
grep -E "QCPU RUNTIME LIMIT GUARD READY|NON_DESTRUCTIVE_RUNTIME_LIMIT_GUARD|QCPU_RUNTIME_MODE" build/qcpu_runtime_limit_guard.md .qcpu/runtime_limits.env

echo
echo "=== TEST QCPU RUNTIME POLICY ENGINE ==="
./scripts/qcpu_runtime_policy_engine.sh
test -f build/qcpu_runtime_policy_engine.md
test -f .qcpu/runtime_policy.env
grep -E "QCPU RUNTIME POLICY ENGINE READY|ALLOW_STANDARD_WORKLOAD|BLOCK_HEAVY_WORKLOAD|NON_DESTRUCTIVE_RUNTIME_POLICY_ENGINE" build/qcpu_runtime_policy_engine.md .qcpu/runtime_policy.env

echo
echo "=== TEST QCPU WORKLOAD ADMISSION CONTROLLER ==="
./scripts/qcpu_workload_admission.sh
test -f build/qcpu_workload_admission.md
test -f .qcpu/workload_admission.env
grep -E "QCPU WORKLOAD ADMISSION CONTROLLER READY|ADMIT_WORKLOAD|REJECT_WORKLOAD|NON_DESTRUCTIVE_WORKLOAD_ADMISSION_CONTROLLER" build/qcpu_workload_admission.md .qcpu/workload_admission.env

echo
echo "=== TEST QCPU WORKLOAD EXECUTION WRAPPER ==="
./scripts/qcpu_workload_execute.sh
test -f build/qcpu_workload_execution.md
test -f .qcpu/workload_execution.env
grep -E "QCPU WORKLOAD EXECUTION WRAPPER READY|STANDARD_WORKLOAD_EXECUTED|HEAVY_WORKLOAD_NOT_EXECUTED|NON_DESTRUCTIVE_WORKLOAD_EXECUTION_WRAPPER" build/qcpu_workload_execution.md .qcpu/workload_execution.env

echo
echo "=== TEST QCPU CI EVIDENCE GATE ==="
chmod +x scripts/qcpu_ci_evidence_gate.sh
./scripts/qcpu_ci_evidence_gate.sh
grep -F "QCPU CI EVIDENCE GATE READY" build/qcpu_ci_evidence_gate.md .qcpu/ci_evidence_gate.env
grep -F "PASS: CI_EVIDENCE_GATE_OPENED" build/qcpu_ci_evidence_gate.md .qcpu/ci_evidence_gate.env

echo
echo "=== TEST QCPU RELEASE READINESS SEAL ==="
chmod +x scripts/qcpu_release_readiness_seal.sh
./scripts/qcpu_release_readiness_seal.sh
grep -F "QCPU RELEASE READINESS SEAL READY" build/qcpu_release_readiness_seal.md
grep -F "QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY" .qcpu/release_readiness_seal.env

echo
echo "=== TEST QCPU PUBLIC RELEASE MANIFEST ==="
chmod +x scripts/qcpu_public_release_manifest.sh
./scripts/qcpu_public_release_manifest.sh
grep -F "QCPU PUBLIC RELEASE MANIFEST READY" build/qcpu_public_release_manifest.md
grep -F "QCPU_PUBLIC_MANIFEST_STATUS=PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY" .qcpu/public_release_manifest.env

echo
echo "=== TEST QCPU CI EVIDENCE REPORTER ==="
chmod +x scripts/qcpu_ci_evidence.sh
./scripts/qcpu_ci_evidence.sh
grep -R "QCPU CI EVIDENCE REPORTER READY" build/qcpu_ci_evidence.md .qcpu/ci_evidence.env

echo
echo
echo "=== TEST QCPU PUBLIC DEMO RUNTIME ==="
chmod +x scripts/qcpu_public_demo.sh
./scripts/qcpu_public_demo.sh
grep -F "QCPU PUBLIC DEMO RUNTIME READY" build/qcpu_public_demo_runtime.md
grep -F "QCPU_PUBLIC_DEMO_STATUS=PASS: QCPU_PUBLIC_DEMO_RUNTIME_READY" .qcpu/public_demo_runtime.env

echo
echo "=== TEST QNOVA PUBLIC DEMO CLI ==="
chmod +x scripts/qnova_demo.sh
./scripts/qnova_demo.sh
grep -F "QNOVA PUBLIC DEMO CLI READY" build/qnova_public_demo_cli.md .qcpu/qnova_demo.env
grep -F "PASS: QNOVA_PUBLIC_DEMO_CLI_READY" build/qnova_public_demo_cli.md .qcpu/qnova_demo.env


echo
echo "=== QNOVA PUBLIC INSTALL CHECK ==="

bash -n install.sh

./install.sh --dry-run > /tmp/qnova_install_dryrun.txt
cat /tmp/qnova_install_dryrun.txt

grep -F "PASS: QNOVA_INSTALL_DRY_RUN_READY" /tmp/qnova_install_dryrun.txt
grep -F "QNOVA PUBLIC INSTALL SCRIPT READY" /tmp/qnova_install_dryrun.txt
grep -F "PASS: QNOVA_PUBLIC_INSTALL_READY" docs/QNOVA_PUBLIC_INSTALL.md
grep -F "QNOVA Public Install Script" README.md

echo "PASS: QNOVA_PUBLIC_INSTALL_READY"

echo "ALL QBIT NOVA TESTS PASSED"
