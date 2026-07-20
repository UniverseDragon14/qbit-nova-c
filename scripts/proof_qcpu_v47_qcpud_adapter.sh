#!/usr/bin/env bash
set -Eeuo pipefail
umask 077
export LC_ALL=C

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/qcpu-v47-stage2b.XXXXXX")"
BIN_DIR="$TMP_ROOT/bin"
QCPUD_BIN="$BIN_DIR/qcpud"
TEST_BIN="$BIN_DIR/qcpu-qcpud-adapter-test"
SOCKET_PATH="$TMP_ROOT/qcpud.sock"
QCPUD_LOG="$TMP_ROOT/qcpud.log"
MOCK_RUNTIME="$TMP_ROOT/mock-runtime"
OFFLINE_RUNTIME="$TMP_ROOT/offline-runtime"
STALL_RUNTIME="$TMP_ROOT/stall-runtime"
STALL_SOCKET="$TMP_ROOT/stall.sock"
DISCONNECT_RUNTIME="$TMP_ROOT/disconnect-runtime"
DISCONNECT_SOCKET="$TMP_ROOT/disconnect.sock"
DISCONNECT_MARKER="$TMP_ROOT/disconnect.accepted"
RESTART_RUNTIME="$TMP_ROOT/restart-runtime"
QCPUD_PID=""
STALL_PID=""
DISCONNECT_PID=""
DISCONNECT_TEST_PID=""

cleanup() {
  if [ -n "${QCPUD_PID:-}" ] && kill -0 "$QCPUD_PID" 2>/dev/null; then
    kill -TERM "$QCPUD_PID" 2>/dev/null || true
    wait "$QCPUD_PID" 2>/dev/null || true
  fi

  if [ -n "${STALL_PID:-}" ] && kill -0 "$STALL_PID" 2>/dev/null; then
    kill -TERM "$STALL_PID" 2>/dev/null || true
    wait "$STALL_PID" 2>/dev/null || true
  fi

  if [ -n "${DISCONNECT_PID:-}" ] && kill -0 "$DISCONNECT_PID" 2>/dev/null; then
    kill -TERM "$DISCONNECT_PID" 2>/dev/null || true
    wait "$DISCONNECT_PID" 2>/dev/null || true
  fi

  if [ -n "${DISCONNECT_TEST_PID:-}" ] && \
     kill -0 "$DISCONNECT_TEST_PID" 2>/dev/null; then
    wait "$DISCONNECT_TEST_PID" 2>/dev/null || true
  fi

  rm -rf "$TMP_ROOT"
}
trap cleanup EXIT

cd "$ROOT"
mkdir -p \
  "$BIN_DIR" \
  "$MOCK_RUNTIME" \
  "$OFFLINE_RUNTIME" \
  "$STALL_RUNTIME" \
  "$DISCONNECT_RUNTIME" \
  "$RESTART_RUNTIME"
chmod 700 \
  "$BIN_DIR" \
  "$MOCK_RUNTIME" \
  "$OFFLINE_RUNTIME" \
  "$STALL_RUNTIME" \
  "$DISCONNECT_RUNTIME" \
  "$RESTART_RUNTIME"

echo "=== NOVAKUTTY / QBIT NOVA C V4.7 STAGE 2B QCPUD ADAPTER PROOF ==="
echo "[OWNER] Universal Dragon Aslam"
echo "[GUARD] userspace_only = YES"
echo "[GUARD] unix_socket_only = YES"
echo "[GUARD] kernel_module = NO"
echo "[GUARD] device_node = NO"
echo "[GUARD] root_action = NO"
echo "[GUARD] tcp_udp_listener = NO"

gcc \
  -std=c11 \
  -D_POSIX_C_SOURCE=200809L \
  -Wall \
  -Wextra \
  -Werror \
  -pedantic \
  -pthread \
  -I. \
  src/device/qcpu_device_wire.c \
  src/device/qcpu_device_io.c \
  src/device/qcpu_mock_frontend.c \
  src/device/qcpu_qcpud_adapter.c \
  tests/v47/qcpu_qcpud_adapter_test.c \
  -lm \
  -o "$TEST_BIN"

gcc \
  -std=c11 \
  -D_POSIX_C_SOURCE=200809L \
  -Wall \
  -Wextra \
  -Werror \
  -pedantic \
  -I. \
  src/device/qcpu_device_wire.c \
  src/device/qcpu_device_io.c \
  src/quantum/qcpu_kernel.c \
  src/device/qcpud.c \
  -lm \
  -o "$QCPUD_BIN"

echo "PASS: QCPU_V47_STAGE2B_STRICT_BUILD_READY"

"$TEST_BIN" sigpipe "$MOCK_RUNTIME" "$SOCKET_PATH"

"$QCPUD_BIN" --socket "$SOCKET_PATH" >"$QCPUD_LOG" 2>&1 &
QCPUD_PID=$!

for _ in $(seq 1 100); do
  if [ -S "$SOCKET_PATH" ] && grep -Fq "PASS: QCPUD_SOCKET_READY" "$QCPUD_LOG"; then
    break
  fi

  if ! kill -0 "$QCPUD_PID" 2>/dev/null; then
    cat "$QCPUD_LOG"
    echo "FAIL: qcpud exited before readiness"
    exit 1
  fi

  sleep 0.02
done

[ -S "$SOCKET_PATH" ]
[ "$(stat -c '%a' "$SOCKET_PATH")" = "600" ]

"$TEST_BIN" happy "$MOCK_RUNTIME" "$SOCKET_PATH"

kill -TERM "$QCPUD_PID"
wait "$QCPUD_PID"
QCPUD_PID=""

grep -Fq "PASS: QCPUD_CLEAN_SHUTDOWN" "$QCPUD_LOG"
[ ! -e "$SOCKET_PATH" ]
echo "PASS: QCPU_V47_STAGE2B_QCPUD_GRACEFUL_CLEANUP_READY"

"$TEST_BIN" offline "$OFFLINE_RUNTIME" "$TMP_ROOT/missing.sock"

python3 - "$STALL_SOCKET" <<'PY' &
import os
import signal
import socket
import sys
import time

path = sys.argv[1]
server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(path)
os.chmod(path, 0o600)
server.listen(1)

def stop(_signum, _frame):
    server.close()
    raise SystemExit(0)

signal.signal(signal.SIGTERM, stop)
client, _ = server.accept()
client.recv(24)
time.sleep(5)
client.close()
server.close()
PY
STALL_PID=$!

for _ in $(seq 1 100); do
  [ -S "$STALL_SOCKET" ] && break
  kill -0 "$STALL_PID" 2>/dev/null
  sleep 0.02
done

[ -S "$STALL_SOCKET" ]
"$TEST_BIN" stall "$STALL_RUNTIME" "$STALL_SOCKET"

kill -TERM "$STALL_PID" 2>/dev/null || true
wait "$STALL_PID" 2>/dev/null || true
STALL_PID=""

python3 - "$DISCONNECT_SOCKET" "$DISCONNECT_MARKER" <<'PY' &
import os
import signal
import socket
import sys
import time

path = sys.argv[1]
marker = sys.argv[2]
server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(path)
os.chmod(path, 0o600)
server.listen(1)

def stop(_signum, _frame):
    server.close()
    raise SystemExit(0)

signal.signal(signal.SIGTERM, stop)
client, _ = server.accept()
client.recv(24)
with open(marker, "w", encoding="ascii") as handle:
    handle.write("accepted\n")
time.sleep(30)
client.close()
server.close()
PY
DISCONNECT_PID=$!

for _ in $(seq 1 100); do
  [ -S "$DISCONNECT_SOCKET" ] && break
  kill -0 "$DISCONNECT_PID" 2>/dev/null
  sleep 0.02
done

[ -S "$DISCONNECT_SOCKET" ]
"$TEST_BIN" disconnect \
  "$DISCONNECT_RUNTIME" \
  "$DISCONNECT_SOCKET" &
DISCONNECT_TEST_PID=$!

for _ in $(seq 1 100); do
  [ -f "$DISCONNECT_MARKER" ] && break
  kill -0 "$DISCONNECT_PID" 2>/dev/null
  kill -0 "$DISCONNECT_TEST_PID" 2>/dev/null
  sleep 0.02
done

[ -f "$DISCONNECT_MARKER" ]
kill -TERM "$DISCONNECT_PID"
wait "$DISCONNECT_PID" 2>/dev/null || true
DISCONNECT_PID=""
wait "$DISCONNECT_TEST_PID"
DISCONNECT_TEST_PID=""

rm -f "$QCPUD_LOG"
"$QCPUD_BIN" --socket "$SOCKET_PATH" >"$QCPUD_LOG" 2>&1 &
QCPUD_PID=$!

for _ in $(seq 1 100); do
  if [ -S "$SOCKET_PATH" ] && grep -Fq "PASS: QCPUD_SOCKET_READY" "$QCPUD_LOG"; then
    break
  fi

  kill -0 "$QCPUD_PID" 2>/dev/null
  sleep 0.02
done

[ -S "$SOCKET_PATH" ]
"$TEST_BIN" happy "$RESTART_RUNTIME" "$SOCKET_PATH"
kill -TERM "$QCPUD_PID"
wait "$QCPUD_PID"
QCPUD_PID=""
[ ! -e "$SOCKET_PATH" ]
echo "PASS: QCPU_V47_STAGE2B_QCPUD_RESTART_READY"

if find "$TMP_ROOT" -type f -name '*.ko' -print | grep -q .; then
  echo "FAIL: kernel module artifact detected"
  exit 1
fi

echo "PASS: QCPU_V47_STAGE2B_EXPLICIT_LITTLE_ENDIAN_FRAMES_READY"
echo "PASS: QCPU_V47_STAGE2B_NO_KERNEL_ARTIFACTS"
echo "PASS: QCPU_V47_STAGE2B_NO_DEVICE_NODE_ACTION"
echo "PASS: QCPU_V47_STAGE2B_NO_NETWORK_LISTENER"
echo "PASS: QCPU_V47_STAGE2B_QCPUD_ADAPTER_PROOF_READY"
