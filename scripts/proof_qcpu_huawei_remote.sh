#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

export LC_ALL=C

ROOT="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
  pwd
)"

cd "$ROOT"

if [ -z "${SSH_CONNECTION:-}" ]; then
  echo "STATUS=FAIL"
  echo "REASON=SSH_REMOTE_CONTROL_CHANNEL_NOT_PRESENT"
  exit 1
fi

HOST_ARCH="$(uname -m)"
HOST_NAME="$(hostname)"
HOST_MODEL="UNKNOWN"

if [ -r /proc/device-tree/model ]; then
  HOST_MODEL="$(tr -d '\0' < /proc/device-tree/model)"
fi

case "$HOST_MODEL" in
  *"Raspberry Pi 5"*) ;;
  *)
    echo "STATUS=FAIL"
    echo "REASON=HOST_IS_NOT_VERIFIED_RASPBERRY_PI_5"
    echo "HOST_MODEL=$HOST_MODEL"
    exit 1
    ;;
esac

case "$HOST_ARCH" in
  aarch64|arm64) ;;
  *)
    echo "STATUS=FAIL"
    echo "REASON=HOST_ARCH_NOT_ARM64"
    echo "HOST_ARCH=$HOST_ARCH"
    exit 1
    ;;
esac

TMP_ROOT="$(
  mktemp -d \
    "${TMPDIR:-/tmp}/qbit-nova-huawei-proof.XXXXXX"
)"

export QCPU_RUNTIME_DIR="$TMP_ROOT/runtime"
export QCPU_BUILD_DIR="$ROOT/build"

LIFECYCLE="$ROOT/scripts/qcpu_device_lifecycle.sh"
CLIENT_LABEL="${QCPU_REMOTE_CLIENT_LABEL:-HUAWEI_TERMUX_USER_ATTESTED}"
REMOTE_SOURCE="$(printf '%s\n' "$SSH_CONNECTION" | awk '{print $1}')"

cleanup() {
  "$LIFECYCLE" stop >/dev/null 2>&1 || true
  rm -rf "$TMP_ROOT"
}

trap cleanup EXIT

"$LIFECYCLE" start >/dev/null

STATUS_OUTPUT="$("$LIFECYCLE" status)"
GHZ_OUTPUT="$("$LIFECYCLE" run-ghz 3 20 424242)"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "TYPE=SOFTWARE_VIRTUAL_QCPU"

printf '%s\n' "$STATUS_OUTPUT" |
grep -Fq "PHYSICAL_QPU_PRESENT=NO"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "BASIS_STATES=8"

printf '%s\n' "$GHZ_OUTPUT" |
grep -Fq "INVALID_RESULTS=0"

"$LIFECYCLE" stop >/dev/null

printf 'REMOTE_CONTROL_TRANSPORT=SSH\n'
printf 'REMOTE_SOURCE=%s\n' "$REMOTE_SOURCE"
printf 'REMOTE_CLIENT_LABEL=%s\n' "$CLIENT_LABEL"
printf 'REMOTE_CLIENT_IDENTITY_VERIFIED_BY_HOST=NO\n'
printf 'REMOTE_CLIENT_IDENTITY_BASIS=USER_ATTESTATION\n'
printf 'HOST_NAME=%s\n' "$HOST_NAME"
printf 'HOST_ARCH=%s\n' "$HOST_ARCH"
printf 'HOST_MODEL=%s\n' "$HOST_MODEL"
printf 'COMPUTE_LOCATION=RASPBERRY_PI_5\n'
printf 'HUAWEI_COMPUTE_ROLE=REMOTE_TERMINAL_ONLY\n'
printf 'PHYSICAL_QPU_PRESENT=NO\n'
printf 'PASS: HUAWEI_TERMUX_TO_PI5_QCPU_REMOTE_CONTROL_PROOF\n'
