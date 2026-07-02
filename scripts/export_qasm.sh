#!/usr/bin/env bash
set -e

INPUT="${1:-examples/bell_qasm.qn}"
OUTPUT="${2:-build/bell.qasm}"

mkdir -p "$(dirname "$OUTPUT")"

echo "=== BUILD QASM EXPORTER ==="
gcc src/tools/qasm_export.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    -o qnova-qasm -lm

echo "=== EXPORT QASM ==="
echo "input:  $INPUT"
echo "output: $OUTPUT"

./qnova-qasm "$INPUT" > "$OUTPUT"

echo "=== QASM FILE CREATED ==="
ls -lh "$OUTPUT"

echo
echo "=== QASM PREVIEW ==="
sed -n '1,40p' "$OUTPUT"
