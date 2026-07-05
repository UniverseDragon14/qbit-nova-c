#!/usr/bin/env bash
set -u

echo "=== QNOVA V4.3 KIMI BRIDGE TEST ==="

python3 -m py_compile scripts/qnova_kimi_bridge.py

OUT="$(python3 scripts/qnova_kimi_bridge.py status)"

echo "$OUT"

echo "$OUT" | grep -q "provider: local_kimi_api_bridge"
echo "$OUT" | grep -Eq "PASS: KIMI_BRIDGE_READY|PASS: KIMI_BRIDGE_DISABLED_NO_KEY"

echo
echo "PASS: QNOVA_KIMI_BRIDGE_TEST_READY"
