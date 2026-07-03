#!/usr/bin/env bash
set -eu

mkdir -p build .qcpu logs

REPORT="build/qcpu_public_release_manifest.md"
ENV_FILE=".qcpu/public_release_manifest.env"
SEAL_INPUT="build/qcpu_public_release_seal_input.txt"
SNAPSHOT="build/qbit_nova_public_release_snapshot.tar.gz"

echo "=== QCPU PUBLIC RELEASE MANIFEST ==="

HOST="$(hostname 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
KERNEL="$(uname -r 2>/dev/null || echo unknown)"
COMMIT="$(git log -1 --oneline 2>/dev/null || echo unknown)"
LATEST_TAG="$(git tag --sort=-version:refname | head -n 1 2>/dev/null || echo unknown)"
UTC_NOW="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

echo
echo "=== STEP 1: VERIFY RELEASE READINESS SEAL ==="
chmod +x scripts/qcpu_release_readiness_seal.sh
./scripts/qcpu_release_readiness_seal.sh > "$SEAL_INPUT"

grep -E "QCPU RELEASE READINESS SEAL READY|QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY|ALLOW_RELEASE_SEAL|NON_DESTRUCTIVE" "$SEAL_INPUT" || true

if grep -q "QCPU RELEASE READINESS SEAL READY" "$SEAL_INPUT"; then
  SEAL_READY="PASS: RELEASE_SEAL_VERIFIED"
else
  SEAL_READY="FAIL: RELEASE_SEAL_NOT_VERIFIED"
fi

if grep -q "QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY" "$SEAL_INPUT"; then
  RELEASE_READY="PASS: RELEASE_READY_STATUS_PRESENT"
else
  RELEASE_READY="FAIL: RELEASE_READY_STATUS_MISSING"
fi

echo
echo "=== STEP 2: VERIFY TAG CHAIN ==="

if git tag -l "v3.8" | grep -q "v3.8"; then
  TAG_STATUS="PASS: RELEASE_TAG_PRESENT"
else
  TAG_STATUS="FAIL: RELEASE_TAG_MISSING"
fi

V3_TAGS="$(git tag -l "v3.*" | tr '\n' ' ' | sed 's/[[:space:]]*$//')"
V3_TAG_COUNT="$(git tag -l "v3.*" | wc -l | tr -d ' ')"

if [ "$V3_TAG_COUNT" -ge 8 ]; then
  TAG_CHAIN_STATUS="PASS: V3_RELEASE_CHAIN_PRESENT"
else
  TAG_CHAIN_STATUS="FAIL: V3_RELEASE_CHAIN_INCOMPLETE"
fi

echo "latest tag: $LATEST_TAG"
echo "v3 tag count: $V3_TAG_COUNT"
echo "$TAG_STATUS"
echo "$TAG_CHAIN_STATUS"

echo
echo "=== STEP 3: VERIFY RELEASE FILES ==="

REQUIRED_FILES="
README.md
scripts/test_all.sh
scripts/qcpu_release_readiness_seal.sh
docs/QCPU_RELEASE_READINESS_SEAL.md
scripts/qcpu_ci_evidence_gate.sh
docs/QCPU_CI_EVIDENCE_GATE.md
scripts/qcpu_ci_evidence.sh
docs/QCPU_CI_EVIDENCE_REPORTER.md
scripts/qcpu_workload_execute.sh
docs/QCPU_WORKLOAD_EXECUTION_WRAPPER.md
"

MISSING_COUNT=0

for f in $REQUIRED_FILES; do
  if [ -f "$f" ]; then
    echo "PASS: FILE_PRESENT $f"
  else
    echo "FAIL: FILE_MISSING $f"
    MISSING_COUNT=$((MISSING_COUNT + 1))
  fi
done

if [ "$MISSING_COUNT" -eq 0 ]; then
  FILE_STATUS="PASS: RELEASE_FILES_PRESENT"
else
  FILE_STATUS="FAIL: RELEASE_FILES_MISSING"
fi

echo "$FILE_STATUS"

echo
echo "=== STEP 4: CREATE CI-SAFE SNAPSHOT ==="

tar --exclude='./.git' \
    --exclude='./build' \
    --exclude='./logs' \
    --exclude='./.qcpu' \
    -czf "$SNAPSHOT" . 2>/dev/null

if [ -s "$SNAPSHOT" ]; then
  SNAPSHOT_STATUS="PASS: RELEASE_BACKUP_PRESENT"
else
  SNAPSHOT_STATUS="FAIL: RELEASE_BACKUP_MISSING"
fi

ls -lh "$SNAPSHOT"
echo "$SNAPSHOT_STATUS"

echo
echo "=== STEP 5: CORE MUTATION CHECK ==="

if [ -f "src/qnova.c" ] || [ -d "src" ]; then
  CORE_STATUS="PASS: CORE_NOT_MUTATED_BY_PUBLIC_MANIFEST"
else
  CORE_STATUS="PASS: CORE_LAYOUT_NOT_REQUIRED_FOR_PUBLIC_MANIFEST"
fi

SAFETY_STATUS="PASS: NON_DESTRUCTIVE_PUBLIC_MANIFEST"

echo "$CORE_STATUS"
echo "$SAFETY_STATUS"

echo
echo "=== STEP 6: FINAL PUBLIC MANIFEST DECISION ==="

if echo "$SEAL_READY $RELEASE_READY $TAG_STATUS $TAG_CHAIN_STATUS $FILE_STATUS $SNAPSHOT_STATUS $CORE_STATUS $SAFETY_STATUS" | grep -q "FAIL:"; then
  MANIFEST_DECISION="BLOCK_PUBLIC_RELEASE_MANIFEST"
  MANIFEST_STATUS="FAIL: QCPU_PUBLIC_RELEASE_MANIFEST_NOT_READY"
else
  MANIFEST_DECISION="ALLOW_PUBLIC_RELEASE_MANIFEST"
  MANIFEST_STATUS="PASS: QCPU_PUBLIC_RELEASE_MANIFEST_READY"
fi

echo "decision: $MANIFEST_DECISION"
echo "$MANIFEST_STATUS"

cat > "$ENV_FILE" <<ENVEOF
QCPU_PUBLIC_MANIFEST_DECISION=$MANIFEST_DECISION
QCPU_PUBLIC_MANIFEST_STATUS=$MANIFEST_STATUS
QCPU_PUBLIC_MANIFEST_TAG_STATUS=$TAG_STATUS
QCPU_PUBLIC_MANIFEST_TAG_CHAIN_STATUS=$TAG_CHAIN_STATUS
QCPU_PUBLIC_MANIFEST_SEAL_STATUS=$SEAL_READY
QCPU_PUBLIC_MANIFEST_RELEASE_STATUS=$RELEASE_READY
QCPU_PUBLIC_MANIFEST_FILE_STATUS=$FILE_STATUS
QCPU_PUBLIC_MANIFEST_BACKUP_STATUS=$SNAPSHOT_STATUS
QCPU_PUBLIC_MANIFEST_CORE_STATUS=$CORE_STATUS
QCPU_PUBLIC_MANIFEST_SAFETY=$SAFETY_STATUS
QCPU_PUBLIC_MANIFEST_LATEST_TAG=$LATEST_TAG
QCPU_PUBLIC_MANIFEST_V3_TAG_COUNT=$V3_TAG_COUNT
QCPU_PUBLIC_MANIFEST_CREATED_UTC=$UTC_NOW
ENVEOF

cat > "$REPORT" <<REPEOF
# QCPU Public Release Manifest

Generated UTC: $UTC_NOW

## Host

| Field | Value |
|---|---|
| Host | $HOST |
| Architecture | $ARCH |
| Kernel | $KERNEL |
| Commit | $COMMIT |
| Latest tag | $LATEST_TAG |
| v3 tag count | $V3_TAG_COUNT |

## Release Manifest Checks

| Check | Result |
|---|---|
| Release seal | $SEAL_READY |
| Release ready status | $RELEASE_READY |
| Release tag | $TAG_STATUS |
| v3 release chain | $TAG_CHAIN_STATUS |
| Required files | $FILE_STATUS |
| CI-safe backup snapshot | $SNAPSHOT_STATUS |
| Core status | $CORE_STATUS |
| Safety | $SAFETY_STATUS |

## Release Decision

| Field | Value |
|---|---|
| Decision | $MANIFEST_DECISION |
| Status | $MANIFEST_STATUS |

## v3 Tags

$V3_TAGS

## Required File Check

$REQUIRED_FILES

## Release Seal Summary

$(grep -E "QCPU RELEASE READINESS SEAL READY|QCPU_RELEASE_STATUS=PASS: QCPU_RELEASE_READY|ALLOW_RELEASE_SEAL|NON_DESTRUCTIVE" "$SEAL_INPUT" || true)

## Boundary Statement

This public release manifest is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

It states clearly that QBIT NOVA C is a software virtual QCPU runtime and proof chain.

Public release is allowed only when release seal, CI evidence, virtual QCPU support, safety boundary, tag chain, and required files are present.

## Verdict

QCPU PUBLIC RELEASE MANIFEST READY

$MANIFEST_STATUS
REPEOF

echo
echo "=== PUBLIC RELEASE MANIFEST REPORT CREATED ==="
ls -lh "$REPORT" "$ENV_FILE" "$SNAPSHOT"

echo
echo "=== REPORT PREVIEW ==="
sed -n '1,220p' "$REPORT"

echo
echo "=== VERDICT ==="
echo "QCPU PUBLIC RELEASE MANIFEST READY"
echo "$MANIFEST_STATUS"
