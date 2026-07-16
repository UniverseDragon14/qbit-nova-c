#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

BUILD_DIR="${QCPU_BUILD_DIR:-$ROOT/build}"
RUNTIME_DIR="${QCPU_RUNTIME_DIR:-${XDG_RUNTIME_DIR:-$HOME/.local/run}/qbit-nova}"
SOCKET_PATH="$RUNTIME_DIR/qcpu.sock"
PID_FILE="$RUNTIME_DIR/qcpud.pid"
LOG_FILE="$RUNTIME_DIR/qcpud.log"
QCPUD_BIN="$BUILD_DIR/qcpud"
CLIENT_BIN="$BUILD_DIR/qnova-device"

fail() {
  printf 'STATUS=FAIL\nREASON=%s\n' "$1" >&2
  exit 1
}

safe_runtime_dir() {
  if [ -L "$RUNTIME_DIR" ]; then
    fail "RUNTIME_DIR_IS_SYMLINK"
  fi

  mkdir -p "$RUNTIME_DIR"

  if [ ! -d "$RUNTIME_DIR" ]; then
    fail "RUNTIME_DIR_NOT_DIRECTORY"
  fi

  if [ "$(stat -c '%u' "$RUNTIME_DIR")" != "$(id -u)" ]; then
    fail "RUNTIME_DIR_NOT_OWNER_CONTROLLED"
  fi

  chmod 700 "$RUNTIME_DIR"
}

safe_existing_file() {
  local path="$1"

  if [ ! -e "$path" ]; then
    return 0
  fi

  if [ -L "$path" ]; then
    fail "UNSAFE_SYMLINK:$path"
  fi

  if [ ! -f "$path" ]; then
    fail "UNSAFE_NON_REGULAR_FILE:$path"
  fi

  if [ "$(stat -c '%u' "$path")" != "$(id -u)" ]; then
    fail "UNSAFE_FILE_OWNER:$path"
  fi
}

safe_existing_socket() {
  if [ ! -e "$SOCKET_PATH" ]; then
    return 0
  fi

  if [ ! -S "$SOCKET_PATH" ]; then
    fail "UNSAFE_SOCKET_PATH_TYPE"
  fi

  if [ "$(stat -c '%u' "$SOCKET_PATH")" != "$(id -u)" ]; then
    fail "UNSAFE_SOCKET_OWNER"
  fi
}

build_device() {
  mkdir -p "$BUILD_DIR"

  local cflags=(
    -std=c11
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -O2
    -Isrc
  )

  (
    cd "$ROOT"

    gcc \
      "${cflags[@]}" \
      src/device/qcpu_device_wire.c \
      src/quantum/qcpu_kernel.c \
      src/device/qcpud.c \
      -lm \
      -o "$QCPUD_BIN"

    gcc \
      "${cflags[@]}" \
      src/device/qcpu_device_wire.c \
      src/tools/qnova_device_client.c \
      -o "$CLIENT_BIN"
  )

  chmod 700 "$QCPUD_BIN" "$CLIENT_BIN"

  echo "PASS: QCPU_DEVICE_LIFECYCLE_BUILD_READY"
}

read_pid() {
  local pid

  safe_existing_file "$PID_FILE"

  if [ ! -f "$PID_FILE" ]; then
    return 1
  fi

  pid="$(cat "$PID_FILE")"

  if ! [[ "$pid" =~ ^[0-9]+$ ]] || [ "$pid" -le 1 ]; then
    fail "INVALID_PID_FILE"
  fi

  printf '%s\n' "$pid"
}

process_matches_qcpud() {
  local pid="$1"
  local process_owner
  local -a command_parts=()

  if ! kill -0 "$pid" 2>/dev/null; then
    return 1
  fi

  if [ ! -r "/proc/$pid/cmdline" ]; then
    return 1
  fi

  process_owner="$(stat -c '%u' "/proc/$pid" 2>/dev/null || true)"

  if [ "$process_owner" != "$(id -u)" ]; then
    return 1
  fi

  while IFS= read -r -d '' part; do
    command_parts+=("$part")
  done < "/proc/$pid/cmdline"

  if [ "${#command_parts[@]}" -ne 3 ]; then
    return 1
  fi

  if [ "${command_parts[0]}" != "$QCPUD_BIN" ]; then
    return 1
  fi

  if [ "${command_parts[1]}" != "--socket" ]; then
    return 1
  fi

  if [ "${command_parts[2]}" != "$SOCKET_PATH" ]; then
    return 1
  fi

  return 0
}

require_running_pid() {
  local pid

  if ! pid="$(read_pid)"; then
    fail "QCPUD_NOT_RUNNING"
  fi

  if ! process_matches_qcpud "$pid"; then
    if kill -0 "$pid" 2>/dev/null; then
      fail "PID_IDENTITY_MISMATCH"
    fi

    rm -f "$PID_FILE"
    fail "STALE_PID_FILE"
  fi

  if [ ! -S "$SOCKET_PATH" ]; then
    fail "QCPUD_SOCKET_MISSING"
  fi

  printf '%s\n' "$pid"
}

start_daemon() {
  local pid temp_pid_file socket_mode

  safe_runtime_dir
  safe_existing_file "$PID_FILE"
  safe_existing_file "$LOG_FILE"
  safe_existing_socket

  if pid="$(read_pid 2>/dev/null)"; then
    if process_matches_qcpud "$pid"; then
      echo "STATUS=PASS"
      echo "ACTION=START"
      echo "ALREADY_RUNNING=YES"
      echo "PID=$pid"
      echo "SOCKET=$SOCKET_PATH"
      return 0
    fi

    if kill -0 "$pid" 2>/dev/null; then
      fail "PID_IDENTITY_MISMATCH"
    fi

    rm -f "$PID_FILE"
  fi

  if [ -e "$SOCKET_PATH" ]; then
    rm -f "$SOCKET_PATH"
  fi

  build_device

  : > "$LOG_FILE"
  chmod 600 "$LOG_FILE"

  nohup "$QCPUD_BIN" \
    --socket "$SOCKET_PATH" \
    >> "$LOG_FILE" \
    2>&1 \
    < /dev/null &

  pid=$!

  for ((attempt = 0; attempt < 200; ++attempt)); do
    if [ -S "$SOCKET_PATH" ] && kill -0 "$pid" 2>/dev/null; then
      break
    fi

    if ! kill -0 "$pid" 2>/dev/null; then
      cat "$LOG_FILE" >&2 || true
      fail "QCPUD_EXITED_BEFORE_READY"
    fi

    sleep 0.05
  done

  if [ ! -S "$SOCKET_PATH" ]; then
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    fail "QCPUD_SOCKET_READY_TIMEOUT"
  fi

  if ! process_matches_qcpud "$pid"; then
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    fail "QCPUD_PROCESS_IDENTITY_FAILED"
  fi

  socket_mode="$(stat -c '%a' "$SOCKET_PATH")"

  if [ "$socket_mode" != "600" ]; then
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    fail "QCPUD_SOCKET_MODE_NOT_600"
  fi

  temp_pid_file="$PID_FILE.tmp.$$"
  printf '%s\n' "$pid" > "$temp_pid_file"
  chmod 600 "$temp_pid_file"
  mv -f "$temp_pid_file" "$PID_FILE"

  echo "STATUS=PASS"
  echo "ACTION=START"
  echo "ALREADY_RUNNING=NO"
  echo "PID=$pid"
  echo "SOCKET=$SOCKET_PATH"
  echo "SOCKET_MODE=$socket_mode"
  echo "LOG=$LOG_FILE"
  echo "PASS: QCPUD_SESSION_DAEMON_STARTED"
}

status_daemon() {
  local pid output

  safe_runtime_dir
  pid="$(require_running_pid)"

  output="$(
    timeout 10 \
      "$CLIENT_BIN" \
      --socket "$SOCKET_PATH" \
      status
  )"

  printf '%s\n' "$output"
  echo "RUNNING=YES"
  echo "PID=$pid"
  echo "SOCKET=$SOCKET_PATH"
  echo "PASS: QCPUD_SESSION_DAEMON_STATUS_READY"
}

run_ghz() {
  local qubits="$1"
  local shots="$2"
  local seed="$3"
  local pid

  safe_runtime_dir
  pid="$(require_running_pid)"

  timeout 30 \
    "$CLIENT_BIN" \
    --socket "$SOCKET_PATH" \
    run-ghz \
    "$qubits" \
    "$shots" \
    "$seed"

  echo "RUNNING=YES"
  echo "PID=$pid"
  echo "PASS: QCPUD_SESSION_GHZ_REQUEST_READY"
}

stop_daemon() {
  local pid stopped=0

  safe_runtime_dir

  if ! pid="$(read_pid)"; then
    safe_existing_socket

    if [ -e "$SOCKET_PATH" ]; then
      rm -f "$SOCKET_PATH"
    fi

    echo "STATUS=PASS"
    echo "ACTION=STOP"
    echo "ALREADY_STOPPED=YES"
    return 0
  fi

  if ! process_matches_qcpud "$pid"; then
    if kill -0 "$pid" 2>/dev/null; then
      fail "PID_IDENTITY_MISMATCH"
    fi

    rm -f "$PID_FILE"
    safe_existing_socket

    if [ -e "$SOCKET_PATH" ]; then
      rm -f "$SOCKET_PATH"
    fi

    echo "STATUS=PASS"
    echo "ACTION=STOP"
    echo "STALE_STATE_CLEANED=YES"
    return 0
  fi

  kill -TERM "$pid"

  for ((attempt = 0; attempt < 100; ++attempt)); do
    if ! kill -0 "$pid" 2>/dev/null; then
      stopped=1
      break
    fi

    sleep 0.05
  done

  if [ "$stopped" -ne 1 ]; then
    echo "STATUS=FAIL"
    echo "REASON=GRACEFUL_STOP_TIMEOUT"
    echo "FORCE_KILL_USED=NO"
    exit 1
  fi

  rm -f "$PID_FILE"

  if [ -e "$SOCKET_PATH" ]; then
    safe_existing_socket
    rm -f "$SOCKET_PATH"
    echo "SOCKET_CLEANUP_FALLBACK=YES"
  else
    echo "SOCKET_CLEANUP_FALLBACK=NO"
  fi

  echo "STATUS=PASS"
  echo "ACTION=STOP"
  echo "PID=$pid"
  echo "FORCE_KILL_USED=NO"
  echo "PASS: QCPUD_SESSION_DAEMON_STOPPED"
}

show_paths() {
  echo "ROOT=$ROOT"
  echo "BUILD_DIR=$BUILD_DIR"
  echo "RUNTIME_DIR=$RUNTIME_DIR"
  echo "SOCKET=$SOCKET_PATH"
  echo "PID_FILE=$PID_FILE"
  echo "LOG_FILE=$LOG_FILE"
}

usage() {
  cat <<USAGE
USAGE:
  $0 build
  $0 start
  $0 status
  $0 run-ghz QUBITS SHOTS SEED
  $0 stop
  $0 paths
USAGE
}

command="${1:-}"

case "$command" in
  build)
    [ "$#" -eq 1 ] || fail "INVALID_BUILD_ARGUMENTS"
    build_device
    ;;

  start)
    [ "$#" -eq 1 ] || fail "INVALID_START_ARGUMENTS"
    start_daemon
    ;;

  status)
    [ "$#" -eq 1 ] || fail "INVALID_STATUS_ARGUMENTS"
    status_daemon
    ;;

  run-ghz)
    [ "$#" -eq 4 ] || fail "INVALID_RUN_GHZ_ARGUMENTS"
    run_ghz "$2" "$3" "$4"
    ;;

  stop)
    [ "$#" -eq 1 ] || fail "INVALID_STOP_ARGUMENTS"
    stop_daemon
    ;;

  paths)
    [ "$#" -eq 1 ] || fail "INVALID_PATHS_ARGUMENTS"
    show_paths
    ;;

  *)
    usage >&2
    exit 2
    ;;
esac
