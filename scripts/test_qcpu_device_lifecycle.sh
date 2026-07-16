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

if [ "$PID_THREE" = "$PID_ONE" ]; then
  echo "FAIL: restart unexpectedly reused active PID"
  exit 1
fi

"$LIFECYCLE" status >/dev/null
"$LIFECYCLE" stop >/dev/null

[ ! -e "$PID_FILE" ]
[ ! -e "$SOCKET_PATH" ]

echo "PASS: QCPUD_RESTART_LIFECYCLE_READY"
echo "PASS: QCPU_V46_SESSION_PERSISTENT_DAEMON_READY"
