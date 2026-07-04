#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${QNOVA_BIN_DIR:-$HOME/.local/bin}"
WRAPPER="$BIN_DIR/qnova-demo"
DRY_RUN="0"

if [ "${1:-}" = "--dry-run" ]; then
  DRY_RUN="1"
fi

echo "=== QNOVA PUBLIC INSTALL SCRIPT ==="
echo "root: $ROOT_DIR"
echo "bin:  $BIN_DIR"

if [ ! -f "$ROOT_DIR/scripts/qnova_demo.sh" ]; then
  echo "FAIL: scripts/qnova_demo.sh not found"
  exit 1
fi

if [ ! -f "$ROOT_DIR/scripts/test_all.sh" ]; then
  echo "FAIL: scripts/test_all.sh not found"
  exit 1
fi

chmod +x "$ROOT_DIR/scripts/qnova_demo.sh"
chmod +x "$ROOT_DIR/scripts/test_all.sh"

if [ "$DRY_RUN" = "1" ]; then
  echo "dry-run: no files installed"
  echo "PASS: QNOVA_INSTALL_DRY_RUN_READY"
  echo "QNOVA PUBLIC INSTALL SCRIPT READY"
  exit 0
fi

mkdir -p "$BIN_DIR"

cat > "$WRAPPER" <<WRAP
#!/usr/bin/env bash
set -eu
cd "$ROOT_DIR"
exec "$ROOT_DIR/scripts/qnova_demo.sh" "\$@"
WRAP

chmod +x "$WRAPPER"

echo
echo "=== INSTALLED ==="
echo "command: $WRAPPER"
echo

case ":$PATH:" in
  *":$BIN_DIR:"*)
    echo "PATH status: PASS: BIN_DIR_ALREADY_IN_PATH"
    ;;
  *)
    echo "PATH status: WARN: BIN_DIR_NOT_IN_PATH"
    echo "Run this once:"
    echo "export PATH=\"\$HOME/.local/bin:\$PATH\""
    ;;
esac

echo
echo "Try:"
echo "  qnova-demo"
echo
echo "QNOVA PUBLIC INSTALL SCRIPT READY"
