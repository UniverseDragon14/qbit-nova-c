#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

expect_fail() {
  local name="$1"
  shift
  local log="$TMP/${name//[^a-zA-Z0-9_-]/_}.log"

  if "$@" >"$log" 2>&1; then
    cat "$log" >&2
    fail "$name unexpectedly succeeded"
  fi

  echo "PASS: $name rejected"
}

echo "=== QCPU CORRECTNESS REGRESSION TESTS ==="

echo
echo "=== CIRCUIT OPERAND VALIDATION ==="
gcc src/quantum/qcircuit.c -o "$TMP/qnova-circuit" -lm -std=c11

cat > "$TMP/malformed-cx.qnc" <<'QNC'
qubits 2
cx 1
measure
QNC

cat > "$TMP/malformed-h.qnc" <<'QNC'
qubits 2
h
measure
QNC

expect_fail "circuit missing cx target" "$TMP/qnova-circuit" "$TMP/malformed-cx.qnc"
expect_fail "circuit missing h operand" "$TMP/qnova-circuit" "$TMP/malformed-h.qnc"

echo
echo "=== BELL SHOT VALIDATION ==="
expect_fail "Bell zero shots" env QCPU_BELL_SHOTS=0 bash scripts/qcpu_bell_shots_reporter.sh
expect_fail "Bell negative shots" env QCPU_BELL_SHOTS=-1 bash scripts/qcpu_bell_shots_reporter.sh
expect_fail "Bell nonnumeric shots" env QCPU_BELL_SHOTS=abc bash scripts/qcpu_bell_shots_reporter.sh

echo
echo "=== GHZ INPUT VALIDATION ==="
expect_fail "GHZ N below range" bash scripts/proof_ghz.sh 1 2
expect_fail "GHZ N above range" bash scripts/proof_ghz.sh 17 2
expect_fail "GHZ nonnumeric N" bash scripts/proof_ghz.sh abc 2
expect_fail "GHZ zero runs" bash scripts/proof_ghz.sh 3 0
expect_fail "GHZ nonnumeric runs" bash scripts/proof_ghz.sh 3 abc

echo
echo "=== PHYSICAL CLAIM FAILURE PROPAGATION ==="
CLAIM_ROOT="$TMP/claim-repo"
mkdir -p "$CLAIM_ROOT/scripts" "$CLAIM_ROOT/fakebin"
cp scripts/qcpu_physical_quantum_claim_trial.sh "$CLAIM_ROOT/scripts/"

cat > "$CLAIM_ROOT/fakebin/gcc" <<'FAKE_GCC'
#!/usr/bin/env bash
set -euo pipefail
out=""
while [ "$#" -gt 0 ]; do
  if [ "$1" = "-o" ] && [ "$#" -ge 2 ]; then
    out="$2"
    shift 2
  else
    shift
  fi
done
[ -n "$out" ] || exit 1
cat > "$out" <<'STUB'
#!/usr/bin/env bash
exit 1
STUB
chmod +x "$out"
FAKE_GCC
chmod +x "$CLAIM_ROOT/fakebin/gcc"

CLAIM_LOG="$TMP/physical-claim.log"
if (
  cd "$CLAIM_ROOT"
  PATH="$CLAIM_ROOT/fakebin:$PATH" bash scripts/qcpu_physical_quantum_claim_trial.sh
) >"$CLAIM_LOG" 2>&1; then
  cat "$CLAIM_LOG" >&2
  fail "physical claim trial passed with broken virtual QCPU"
fi

if grep -q '^PASS: HONEST_CLAIM_TRIAL_READY$' "$CLAIM_LOG"; then
  cat "$CLAIM_LOG" >&2
  fail "physical claim trial printed PASS with broken virtual QCPU"
fi

grep -q '^FAIL: HONEST_CLAIM_TRIAL_VIRTUAL_QCPU_BROKEN$' "$CLAIM_LOG" || {
  cat "$CLAIM_LOG" >&2
  fail "physical claim trial did not report virtual QCPU failure"
}
echo "PASS: physical claim failure propagated"

echo
echo "=== INSTALLER DRY-RUN CONTRACT ==="
INSTALL_ROOT="$TMP/install-repo"
mkdir -p "$INSTALL_ROOT/scripts"
cp install.sh "$INSTALL_ROOT/install.sh"
printf '#!/usr/bin/env bash\nexit 0\n' > "$INSTALL_ROOT/scripts/qnova_demo.sh"
printf '#!/usr/bin/env bash\nexit 0\n' > "$INSTALL_ROOT/scripts/test_all.sh"
chmod 0644 "$INSTALL_ROOT/scripts/qnova_demo.sh" "$INSTALL_ROOT/scripts/test_all.sh"

BEFORE_MODES="$(stat -c '%a:%n' "$INSTALL_ROOT/scripts/qnova_demo.sh" "$INSTALL_ROOT/scripts/test_all.sh")"
DRY_LOG="$TMP/install-dry-run.log"
(
  cd "$INSTALL_ROOT"
  QNOVA_BIN_DIR="$INSTALL_ROOT/bin" bash ./install.sh --dry-run
) >"$DRY_LOG" 2>&1
AFTER_MODES="$(stat -c '%a:%n' "$INSTALL_ROOT/scripts/qnova_demo.sh" "$INSTALL_ROOT/scripts/test_all.sh")"

[ "$BEFORE_MODES" = "$AFTER_MODES" ] || fail "dry-run changed script permissions"
grep -q '^PASS: QNOVA_INSTALL_DRY_RUN_READY$' "$DRY_LOG" || fail "dry-run marker missing"
grep -q '^PASS: QNOVA_PUBLIC_INSTALL_READY$' "$DRY_LOG" || fail "standalone install marker missing from dry-run"
echo "PASS: installer dry-run is permission-safe"

REAL_LOG="$TMP/install-real.log"
(
  cd "$INSTALL_ROOT"
  QNOVA_BIN_DIR="$INSTALL_ROOT/bin" bash ./install.sh
) >"$REAL_LOG" 2>&1

grep -q '^PASS: QNOVA_PUBLIC_INSTALL_READY$' "$REAL_LOG" || fail "standalone install marker missing from real install"
test -x "$INSTALL_ROOT/bin/qnova-demo" || fail "installer did not create executable wrapper"
echo "PASS: installer real success marker present"

echo
echo "=== FINAL SUCCESS MARKER ORDER ==="
FINAL_LINE="$(grep -n 'echo "ALL QBIT NOVA TESTS PASSED"' scripts/test_all.sh | tail -n 1 | cut -d: -f1)"
CIRCUIT_LINE="$(grep -n './scripts/proof_qcircuit.sh' scripts/test_all.sh | tail -n 1 | cut -d: -f1)"
REGRESSION_LINE="$(grep -n './tests/test_qcpu_correctness.sh' scripts/test_all.sh | tail -n 1 | cut -d: -f1)"

[ -n "$FINAL_LINE" ] || fail "final all-tests marker missing"
[ -n "$CIRCUIT_LINE" ] || fail "circuit proof invocation missing"
[ -n "$REGRESSION_LINE" ] || fail "correctness regression invocation missing"
[ "$FINAL_LINE" -gt "$CIRCUIT_LINE" ] || fail "all-tests marker appears before circuit proof"
[ "$FINAL_LINE" -gt "$REGRESSION_LINE" ] || fail "all-tests marker appears before correctness regressions"
echo "PASS: all-tests marker is final"

echo
echo "QCPU CORRECTNESS REGRESSION TESTS READY"
echo "PASS: QCPU_CORRECTNESS_REGRESSION_TESTS_READY"
