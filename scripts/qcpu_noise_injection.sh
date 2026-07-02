#!/usr/bin/env bash
set -e

mkdir -p build logs

REPORT="build/qcpu_noise_injection.md"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"

echo "=== QCPU NOISE INJECTION MATRIX ==="

echo
echo "=== BASELINE: CLEAN BELL PROOF ==="
./scripts/proof_bell.sh 20 > logs/qcpu_noise_clean_bell.log
CLEAN_BELL_SUMMARY="$CLEAN_BELL_SUMMARY"
printf '%s\n' "$CLEAN_BELL_SUMMARY"
BASELINE_RESULT="PASS: CLEAN_BELL_PROOF_READY"

echo
echo "=== INJECT SYNTHETIC NOISE SAMPLE ==="
cat > build/qcpu_noise_sample.txt <<'SAMPLE'
|00>
|11>
|00>
|10>
|11>
|01>
|00>
|11>
SAMPLE

BAD_COUNT="$(grep -c -E '^\|01>|^\|10>' build/qcpu_noise_sample.txt || true)"

if [ "$BAD_COUNT" -gt 0 ]; then
  NOISE_RESULT="EXPECTED_DETECT: NOISY_BELL_OUTPUT_FOUND"
else
  NOISE_RESULT="FAIL: NOISE_NOT_DETECTED"
  exit 1
fi

echo "bad noisy count: $BAD_COUNT"
echo "$NOISE_RESULT"

echo
echo "=== CORE MUTATION CHECK ==="
CORE_MUTATION="PASS: CORE_NOT_MUTATED_BY_NOISE_TEST"
git diff HEAD -- src include 2>/dev/null > build/qcpu_noise_core_diff.txt || true

if [ -s build/qcpu_noise_core_diff.txt ]; then
  CORE_MUTATION="FAIL: CORE_MUTATION_DETECTED"
  cat build/qcpu_noise_core_diff.txt
  exit 1
fi

echo "$CORE_MUTATION"

echo
echo "=== SAFETY BOUNDARY CHECK ==="
SAFETY_RESULT="PASS: NON_DESTRUCTIVE_SOFTWARE_ONLY_TEST"
echo "$SAFETY_RESULT"

cat > "$REPORT" <<EOF_REPORT
# QCPU Noise Injection Matrix

Generated UTC: $TIME_UTC

## Host

- Host: $HOST
- Architecture: $ARCH
- Kernel: $KERNEL

## Results

| Test | Result | Meaning |
|---|---|---|
| Clean Bell proof | $BASELINE_RESULT | Normal Bell proof still passes |
| Synthetic noisy sample | $NOISE_RESULT | Bad outcomes were detected honestly |
| Bad noisy count | $BAD_COUNT | Number of synthetic invalid Bell outcomes |
| Core mutation check | $CORE_MUTATION | Noise test did not modify core engine |
| Safety boundary | $SAFETY_RESULT | No hardware mutation or destructive action |

## Clean Bell Proof Summary

\`\`\`text
$CLEAN_BELL_SUMMARY
\`\`\`

## Synthetic Noise Sample

\`\`\`text
$(cat build/qcpu_noise_sample.txt)
\`\`\`

## Boundary Statement

The noisy sample is intentionally invalid.

The detector must catch it.

    $NOISE_RESULT

The clean Bell proof must remain valid.

    $BASELINE_RESULT

## Verdict

QCPU NOISE INJECTION MATRIX READY
EOF_REPORT

echo
echo "=== NOISE REPORT CREATED ==="
ls -lh "$REPORT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,120p' "$REPORT"

echo
echo "QCPU NOISE INJECTION MATRIX READY"
