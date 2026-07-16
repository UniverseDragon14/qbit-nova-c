#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"

TMP_ROOT="$(
  mktemp -d \
    "${TMPDIR:-/tmp}/qbit-nova-v46-lifecycle.XXXXXX"
)"

export QCPU_RUNTIME_DIR="$TMP_ROOT/runtime"
export QCPU_BUILD_DIR="$ROOT/build"

LIFECYCLE="$ROOT/scripts/qcpu_device_lifecycle.sh"
PID_FILE="$QCPU_RUNTIME_DIR/qcpud.pid"
SOCKET_PATH="$QCPU_RUNTIME_DIR/qcpu.sock"
LOG_FILE="$QCPU_RUNTIME_DIR/qcpud.log"

cleanup() {
  "$LIFECYCLE" stop >/dev/null 2>&1 || true

  if [ -n "${CONCURRENT_RUNTIME:-}" ]; then
    QCPU_RUNTIME_DIR="$CONCURRENT_RUNTIME"       "$LIFECYCLE" stop >/dev/null 2>&1 || true
  fi

  rm -rf "$TMP_ROOT"
}

trap cleanup EXIT

START_OUTPUT="$("$LIFECYCLE" start)"
printf '%s\n' "$START_OUTPUT"

printf '%s\n' "$START_OUTPUT" |
grep -Fq "PASS: QCPUD_SESSION_DAEMON_STARTED"

PID_ONE="$(cat "$PID_FILE")"

[ "$(stat -c '%a' "$QCPU_RUNTIME_DIR")" = "700" ]
[ "$(stat -c '%a' "$PID_FILE")" = "600" ]
[ "$(stat -c '%a' "$SOCKET_PATH")" = "600" ]

STATUS_OUTPUT="$("$LIFECYCLE" status)"
printf '%s\n' "$STATUS_OUTPUT"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "TYPE=SOFTWARE_VIRTUAL_QCPU"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "PHYSICAL_QPU_PRESENT=NO"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "PASS: QCPUD_SESSION_DAEMON_STATUS_READY"

GHZ_THREE_OUTPUT="$(
  "$LIFECYCLE" run-ghz 3 20 424242
)"
printf '%s\n' "$GHZ_THREE_OUTPUT"

printf '%s\n' "$GHZ_THREE_OUTPUT" |
grep -Fq "BASIS_STATES=8"

printf '%s\n' "$GHZ_THREE_OUTPUT" |
grep -Fq "INVALID_RESULTS=0"

GHZ_FOUR_OUTPUT="$(
  "$LIFECYCLE" run-ghz 4 20 424243
)"
printf '%s\n' "$GHZ_FOUR_OUTPUT"

printf '%s\n' "$GHZ_FOUR_OUTPUT" |
grep -Fq "BASIS_STATES=16"

printf '%s\n' "$GHZ_FOUR_OUTPUT" |
grep -Fq "INVALID_RESULTS=0"

PID_TWO="$(cat "$PID_FILE")"

if [ "$PID_ONE" != "$PID_TWO" ]; then
  echo "FAIL: daemon PID changed across requests"
  exit 1
fi

echo "PASS: QCPUD_MULTI_REQUEST_SESSION_PERSISTENCE_READY"

STOP_OUTPUT="$("$LIFECYCLE" stop)"
printf '%s\n' "$STOP_OUTPUT"

printf '%s\n' "$STOP_OUTPUT" |
grep -Fq "PASS: QCPUD_SESSION_DAEMON_STOPPED"

[ ! -e "$PID_FILE" ]
[ ! -e "$SOCKET_PATH" ]

grep -Fq "PASS: QCPUD_CLEAN_SHUTDOWN" "$LOG_FILE"

echo "PASS: QCPUD_GRACEFUL_TERM_CLEANUP_READY"

RESTART_OUTPUT="$("$LIFECYCLE" start)"
printf '%s\n' "$RESTART_OUTPUT"

PID_THREE="$(cat "$PID_FILE")"

"$LIFECYCLE" status >/dev/null
"$LIFECYCLE" stop >/dev/null

[ ! -e "$PID_FILE" ]
[ ! -e "$SOCKET_PATH" ]

echo "PASS: QCPUD_RESTART_LIFECYCLE_READY"
echo "PASS: QCPU_V46_SESSION_PERSISTENT_DAEMON_READY"


echo
echo "=== INVALID PID FILE REJECTION ==="

INVALID_RUNTIME="$TMP_ROOT/invalid-pid-runtime"
mkdir -p "$INVALID_RUNTIME"
chmod 700 "$INVALID_RUNTIME"
printf '%s\n' "not-a-pid" > "$INVALID_RUNTIME/qcpud.pid"
chmod 600 "$INVALID_RUNTIME/qcpud.pid"

if (
  QCPU_RUNTIME_DIR="$INVALID_RUNTIME" \
    "$LIFECYCLE" stop
) > "$TMP_ROOT/invalid-pid.log" 2>&1
then
  echo "FAIL: invalid PID file was accepted"
  exit 1
fi

grep -Fq \
  "REASON=INVALID_PID_FILE" \
  "$TMP_ROOT/invalid-pid.log"

echo "PASS: QCPUD_INVALID_PID_FILE_REJECTED"

echo
echo "=== DANGLING SYMLINK REJECTION ==="

SYMLINK_RUNTIME="$TMP_ROOT/symlink-runtime"
SYMLINK_TARGET="$TMP_ROOT/dangling-log-target"
mkdir -p "$SYMLINK_RUNTIME"
chmod 700 "$SYMLINK_RUNTIME"
ln -s "$SYMLINK_TARGET" "$SYMLINK_RUNTIME/qcpud.log"

if (
  QCPU_RUNTIME_DIR="$SYMLINK_RUNTIME" \
    "$LIFECYCLE" start
) > "$TMP_ROOT/symlink.log" 2>&1
then
  echo "FAIL: dangling symlink was accepted"
  exit 1
fi

grep -Fq \
  "UNSAFE_SYMLINK:$SYMLINK_RUNTIME/qcpud.log" \
  "$TMP_ROOT/symlink.log"

[ ! -e "$SYMLINK_TARGET" ]

echo "PASS: QCPUD_DANGLING_SYMLINK_REJECTED"

echo
echo "=== CONCURRENT START SERIALIZATION ==="

CONCURRENT_RUNTIME="$TMP_ROOT/concurrent-runtime"
START_ONE_LOG="$TMP_ROOT/concurrent-start-one.log"
START_TWO_LOG="$TMP_ROOT/concurrent-start-two.log"

QCPU_RUNTIME_DIR="$CONCURRENT_RUNTIME" \
  "$LIFECYCLE" start \
  > "$START_ONE_LOG" \
  2>&1 &
START_ONE_PID=$!

QCPU_RUNTIME_DIR="$CONCURRENT_RUNTIME" \
  "$LIFECYCLE" start \
  > "$START_TWO_LOG" \
  2>&1 &
START_TWO_PID=$!

wait "$START_ONE_PID"
wait "$START_TWO_PID"

cat "$START_ONE_LOG"
cat "$START_TWO_LOG"

cat "$START_ONE_LOG" "$START_TWO_LOG" |
grep -Fq "PASS: QCPUD_SESSION_DAEMON_STARTED"

cat "$START_ONE_LOG" "$START_TWO_LOG" |
grep -Fq "ALREADY_RUNNING=YES"

QCPU_RUNTIME_DIR="$CONCURRENT_RUNTIME" \
  "$LIFECYCLE" status >/dev/null

QCPU_RUNTIME_DIR="$CONCURRENT_RUNTIME" \
  "$LIFECYCLE" stop >/dev/null

echo "PASS: QCPUD_LIFECYCLE_TRANSITIONS_SERIALIZED"
