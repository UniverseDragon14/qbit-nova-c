#!/usr/bin/env bash
set -e

SESSION_DIR=".qcpu"
BUILD_DIR="build"
LOG_DIR="logs"

mkdir -p "$SESSION_DIR" "$BUILD_DIR" "$LOG_DIR"

HOSTNAME_VALUE="$(hostname 2>/dev/null || echo unknown)"
ARCH_VALUE="$(uname -m 2>/dev/null || echo unknown)"
KERNEL_VALUE="$(uname -r 2>/dev/null || echo unknown)"
TIME_VALUE="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

CPU_MODEL="$(grep -m 1 'Model' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ //')"
if [ -z "$CPU_MODEL" ]; then
  CPU_MODEL="$(grep -m 1 'Hardware' /proc/cpuinfo 2>/dev/null | cut -d ':' -f2- | sed 's/^ //')"
fi
if [ -z "$CPU_MODEL" ]; then
  CPU_MODEL="classical-cpu"
fi

cat > "$SESSION_DIR/session.env" <<EOF
QCPU_MODE=virtual
QCPU_RUNTIME=QBIT_NOVA_C
QCPU_HOSTNAME=$HOSTNAME_VALUE
QCPU_ARCH=$ARCH_VALUE
QCPU_KERNEL=$KERNEL_VALUE
QCPU_CPU_MODEL=$CPU_MODEL
QCPU_BOOT_TIME_UTC=$TIME_VALUE
QCPU_BOUNDARY=software_virtual_qcpu_not_physical_quantum_hardware
EOF

cat > "$BUILD_DIR/qcpu_node.json" <<EOF
{
  "node": "QBIT_NOVA_VIRTUAL_QCPU",
  "mode": "virtual",
  "host": "$HOSTNAME_VALUE",
  "arch": "$ARCH_VALUE",
  "kernel": "$KERNEL_VALUE",
  "cpu_model": "$CPU_MODEL",
  "boot_time_utc": "$TIME_VALUE",
  "runtime": "QBIT_NOVA_C",
  "safety_boundary": "software virtual QCPU layer, not physical quantum hardware"
}
EOF

echo "=== QBIT NOVA VIRTUAL QCPU BOOT ===" | tee "$LOG_DIR/qcpu_boot.log"
echo "host: $HOSTNAME_VALUE" | tee -a "$LOG_DIR/qcpu_boot.log"
echo "arch: $ARCH_VALUE" | tee -a "$LOG_DIR/qcpu_boot.log"
echo "kernel: $KERNEL_VALUE" | tee -a "$LOG_DIR/qcpu_boot.log"
echo "cpu: $CPU_MODEL" | tee -a "$LOG_DIR/qcpu_boot.log"
echo "mode: virtual QCPU" | tee -a "$LOG_DIR/qcpu_boot.log"
echo "boundary: software virtual QCPU, not physical quantum hardware" | tee -a "$LOG_DIR/qcpu_boot.log"

echo
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
echo "=== BUILD QASM EXPORTER ==="
gcc src/tools/qasm_export.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    -o qnova-qasm -lm

echo
echo "=== QCPU PROOF 1: Bell correlation ==="
./scripts/proof_bell.sh 10 | tee "$LOG_DIR/qcpu_bell.log"

echo
echo "=== QCPU PROOF 2: QASM file export ==="
./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm | tee "$LOG_DIR/qcpu_qasm_export.log"

echo
echo "=== QCPU NODE READY ==="
echo "session: $SESSION_DIR/session.env"
echo "node:    $BUILD_DIR/qcpu_node.json"
echo "qasm:    build/bell.qasm"
echo "logs:    logs/qcpu_boot.log"
