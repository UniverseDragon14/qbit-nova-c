#!/usr/bin/env bash
set -e

RUNS="${1:-20}"

echo "=== BUILD QBIT NOVA ==="
gcc src/qnova.c \
    src/lexer/lexer.c \
    src/parser/parser.c \
    src/compiler/compiler.c \
    src/vm/byte_vm.c \
    src/vm/state2_vm.c \
    src/quantum/state2.c \
    src/adapter/safe_adapter.c \
    -o qnova -lm

echo
echo "=== BELL STATE CORRELATION PROOF ==="
echo "runs: $RUNS"
echo

count_00=0
count_11=0
bad=0

for i in $(seq 1 "$RUNS"); do
    out="$(./qnova examples/bell_state2.qn)"

    pair="$(printf "%s\n" "$out" \
        | grep -o 'MEASURE pair |[01][01]>' \
        | tail -n 1 \
        | grep -o '[01][01]')"

    case "$pair" in
        00)
            count_00=$((count_00 + 1))
            echo "run $i: |00> OK"
            ;;
        11)
            count_11=$((count_11 + 1))
            echo "run $i: |11> OK"
            ;;
        01|10)
            bad=$((bad + 1))
            echo "run $i: |$pair> FAIL correlation broken"
            ;;
        *)
            bad=$((bad + 1))
            echo "run $i: UNKNOWN FAIL"
            printf "%s\n" "$out"
            ;;
    esac
done

echo
echo "=== RESULT ==="
echo "|00> count: $count_00"
echo "|11> count: $count_11"
echo "bad count: $bad"

if [ "$bad" -ne 0 ]; then
    echo "BELL PROOF FAILED"
    exit 1
fi

echo "BELL PROOF PASSED"
echo "Only |00> and |11> appeared."
