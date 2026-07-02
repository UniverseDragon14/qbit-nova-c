#!/usr/bin/env bash
set -e

mkdir -p build logs

SNAPSHOT="build/hypercube_snapshot.md"
TIME_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
LATEST_TAG="$(git tag --sort=-v:refname | head -n 1 2>/dev/null || echo unknown)"
LATEST_COMMIT="$(git log --oneline -1 2>/dev/null || echo unknown)"

echo "=== NOVA HYPERCUBE SNAPSHOT ==="

echo
echo "=== ENSURE QCPU BOOT ==="
./scripts/qcpu_boot.sh > logs/hypercube_snapshot_qcpu_boot.log

echo
echo "=== RUN BELL PROOF ==="
./scripts/proof_bell.sh 10 > logs/hypercube_snapshot_bell.log

echo
echo "=== EXPORT QASM ==="
./scripts/export_qasm.sh examples/bell_qasm.qn build/bell.qasm > logs/hypercube_snapshot_qasm.log

BELL_RESULT="$(grep -E 'BELL PROOF PASSED|bad count:' logs/hypercube_snapshot_bell.log | tr '\n' ' ')"

cat > "$SNAPSHOT" <<EOF
# NOVA Hypercube Runtime Snapshot

Generated UTC: $TIME_UTC

## Git

Latest tag: $LATEST_TAG

Latest commit:

    $LATEST_COMMIT

## Runtime Identity

\`\`\`json
$(cat build/qcpu_node.json)
\`\`\`

## Session

\`\`\`text
$(cat .qcpu/session.env)
\`\`\`

## Bell Proof Summary

\`\`\`text
$BELL_RESULT
\`\`\`

## QASM Preview

\`\`\`qasm
$(sed -n '1,40p' build/bell.qasm)
\`\`\`

## Safety Boundary

This is a software virtual QCPU / NOVA Hypercube Runtime layer.
It does not claim that the device is physical quantum hardware.
EOF

echo
echo "=== SNAPSHOT CREATED ==="
ls -lh "$SNAPSHOT"

echo
echo "=== SNAPSHOT PREVIEW ==="
sed -n '1,80p' "$SNAPSHOT"

echo
echo "NOVA HYPERCUBE SNAPSHOT READY"
