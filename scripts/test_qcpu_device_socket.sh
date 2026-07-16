#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"
mkdir -p build

CFLAGS=(
  -std=c11
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -O2
  -Isrc
)

gcc \
  "${CFLAGS[@]}" \
  src/device/qcpu_device_wire.c \
  src/device/qcpu_device_io.c \
  src/test_qcpu_device_wire.c \
  -lm \
  -o build/test-qcpu-device-wire

gcc \
  "${CFLAGS[@]}" \
  src/device/qcpu_device_wire.c \
  src/device/qcpu_device_io.c \
  src/quantum/qcpu_kernel.c \
  src/device/qcpud.c \
  -lm \
  -o build/qcpud

gcc \
  "${CFLAGS[@]}" \
  src/device/qcpu_device_wire.c \
  src/device/qcpu_device_io.c \
  src/tools/qnova_device_client.c \
  -o build/qnova-device

WIRE_OUTPUT="$(./build/test-qcpu-device-wire)"
printf '%s\n' "$WIRE_OUTPUT"

printf '%s\n' "$WIRE_OUTPUT" |
grep -Fq "PASS: QCPU_DEVICE_LITTLE_ENDIAN_WIRE_READY"

printf '%s\n' "$WIRE_OUTPUT" |
grep -Fq "PASS: QCPU_DEVICE_BAD_MAGIC_REJECTED"

printf '%s\n' "$WIRE_OUTPUT" |
grep -Fq "PASS: QCPU_DEVICE_BAD_VERSION_REJECTED"

printf '%s\n' "$WIRE_OUTPUT" |
grep -Fq "PASS: QCPU_DEVICE_WIRE_ROUND_TRIP_READY"

printf '%s\n' "$WIRE_OUTPUT" |
grep -Fq "PASS: QCPU_DEVICE_GOLDEN_PACKETS_READY"

TMP_ROOT="$(
  mktemp -d "${TMPDIR:-/tmp}/qbit-nova-v46.XXXXXX"
)"
chmod 700 "$TMP_ROOT"

SOCKET="$TMP_ROOT/qcpu.sock"
STATUS_LOG="$TMP_ROOT/qcpud-status.log"
GHZ_LOG="$TMP_ROOT/qcpud-ghz.log"
DAEMON_PID=""

cleanup() {
  if (
    [ -n "${DAEMON_PID:-}" ] &&
    kill -0 "$DAEMON_PID" 2>/dev/null
  ); then
    kill "$DAEMON_PID" 2>/dev/null || true
    wait "$DAEMON_PID" 2>/dev/null || true
  fi

  rm -rf "$TMP_ROOT"
}

trap cleanup EXIT

start_daemon() {
  local log_file="$1"
  local attempt=0

  rm -f "$SOCKET"

  ./build/qcpud \
    --socket "$SOCKET" \
    --once \
    > "$log_file" \
    2>&1 &

  DAEMON_PID=$!

  while [ "$attempt" -lt 100 ]; do
    if (
      [ -S "$SOCKET" ] &&
      grep -Fq "PASS: QCPUD_SOCKET_READY" "$log_file"
    ); then
      return 0
    fi

    if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
      cat "$log_file"
      echo "FAIL: qcpud exited before socket ready"
      exit 1
    fi

    sleep 0.05
    attempt=$((attempt + 1))
  done

  cat "$log_file"
  echo "FAIL: qcpud socket readiness timeout"
  exit 1
}

start_daemon "$STATUS_LOG"

SOCKET_MODE="$(stat -c '%a' "$SOCKET")"

if [ "$SOCKET_MODE" != "600" ]; then
  echo "FAIL: socket mode expected=600 actual=$SOCKET_MODE"
  exit 1
fi

STATUS_OUTPUT="$(
  timeout 10 \
    ./build/qnova-device \
      --socket "$SOCKET" \
      status
)"
printf '%s\n' "$STATUS_OUTPUT"

wait "$DAEMON_PID"
DAEMON_PID=""

grep -Fq \
  "PASS: QCPUD_REQUEST_SERVED command=1 status=0" \
  "$STATUS_LOG"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "TYPE=SOFTWARE_VIRTUAL_QCPU"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "HOST=CLASSICAL_CPU"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "PHYSICAL_QPU_PRESENT=NO"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "Q0_ORDER=MOST_SIGNIFICANT"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "STATUS=PASS"

if [ -e "$SOCKET" ]; then
  echo "FAIL: socket remained after clean shutdown"
  exit 1
fi

echo "PASS: QCPUD_OWNER_ONLY_STATUS_SOCKET_READY"

start_daemon "$GHZ_LOG"

GHZ_OUTPUT="$(
  timeout 20 \
    ./build/qnova-device \
      --socket "$SOCKET" \
      run-ghz \
      3 \
      20 \
      424242
)"
printf '%s\n' "$GHZ_OUTPUT"

wait "$DAEMON_PID"
DAEMON_PID=""

grep -Fq \
  "PASS: QCPUD_REQUEST_SERVED command=2 status=0" \
  "$GHZ_LOG"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "QUBITS=3"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "BASIS_STATES=8"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "SHOTS=20"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Eq '^MEASURED_STATE=(0|7)$'

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "INVALID_RESULTS=0"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "NORM=1.000000000000"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "STATUS=PASS"

echo "PASS: QCPUD_GHZ_DEVICE_EXECUTION_READY"
echo "PASS: QCPU_V46_UNIX_SOCKET_DEVICE_READY"


echo
echo "=== PERSISTENT DAEMON CLIENT-FAILURE ISOLATION ==="

PERSIST_LOG="$TMP_ROOT/qcpud-persistent.log"
rm -f "$SOCKET"

./build/qcpud \
  --socket "$SOCKET" \
  > "$PERSIST_LOG" \
  2>&1 &

DAEMON_PID=$!

for ((attempt = 0; attempt < 100; ++attempt)); do
  if (
    [ -S "$SOCKET" ] &&
    grep -Fq "PASS: QCPUD_SOCKET_READY" "$PERSIST_LOG"
  ); then
    break
  fi

  if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
    cat "$PERSIST_LOG"
    echo "FAIL: persistent qcpud exited before ready"
    exit 1
  fi

  sleep 0.05
done

if (
  [ ! -S "$SOCKET" ] ||
  ! grep -Fq "PASS: QCPUD_SOCKET_READY" "$PERSIST_LOG"
); then
  echo "FAIL: persistent qcpud readiness timeout"
  exit 1
fi

python3 - "$SOCKET" <<'PYCLIENT'
import socket
import sys

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(sys.argv[1])
client.close()
PYCLIENT

sleep 0.2
kill -0 "$DAEMON_PID"

AFTER_DISCONNECT_OUTPUT="$(
  timeout 10 \
    ./build/qnova-device \
      --socket "$SOCKET" \
      status
)"

printf '%s\n' "$AFTER_DISCONNECT_OUTPUT" |
grep -Fq "STATUS=PASS"

echo "PASS: QCPUD_CLIENT_DISCONNECT_ISOLATED"

python3 - "$SOCKET" <<'PYSTALL' &
import socket
import sys
import time

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(sys.argv[1])
client.sendall(b"\x00")
time.sleep(3.0)
client.close()
PYSTALL

STALL_PID=$!
wait "$STALL_PID"

sleep 0.2
kill -0 "$DAEMON_PID"

AFTER_TIMEOUT_OUTPUT="$(
  timeout 10 \
    ./build/qnova-device \
      --socket "$SOCKET" \
      status
)"

printf '%s\n' "$AFTER_TIMEOUT_OUTPUT" |
grep -Fq "STATUS=PASS"

echo "PASS: QCPUD_BOUNDED_IO_TIMEOUT_READY"

kill -TERM "$DAEMON_PID"
wait "$DAEMON_PID"
DAEMON_PID=""

grep -Fq \
  "PASS: QCPUD_CLEAN_SHUTDOWN" \
  "$PERSIST_LOG"

[ ! -e "$SOCKET" ]

echo "PASS: QCPUD_PERSISTENT_CLIENT_FAILURE_RECOVERY_READY"
