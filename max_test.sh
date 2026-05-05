#!/bin/bash
#
# MAX TEST - One file, all flags, max detection power
#

if [ -z "$1" ]; then
    echo "Usage: $0 <source.c>"
    echo ""
    echo "Tests ONE file with ALL c_tester flags:"
    echo "  - Default (ASan + UBSan)"
    echo "  - --analyzer (GCC static)"
    echo "  - --valgrind (deep memory)"
    echo ""
    echo "Examples:"
    echo "  $0 my_buggy.c"
    echo "  $0 /path/to/problem.c"
    exit 1
fi

FILE=$1
C_TESTER="./c_tester"

if [ ! -f "$FILE" ]; then
    echo "Error: File not found: $FILE"
    exit 1
fi

echo "========================================"
echo "  c_tester - MAX POWER TEST"
echo "========================================"
echo ""
echo "Testing: $FILE"
echo ""

# Default (ASan + UBSan)
echo "========================================"
echo "[1] DEFAULT (ASan + UBSan)"
echo "========================================"
OUTPUT=$($C_TESTER $FILE 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    echo "$OUTPUT" | grep -E "^\[ERROR\]|\[OK\]" | head -5
    echo "$OUTPUT" | grep -E "Fix:" | head -3
else
    echo "$OUTPUT" | grep -E "^\[OK\]|\[COMPILE"
fi
echo ""

# --analyzer
echo "========================================"
echo "[2] --ANALYZER (GCC static)"
echo "========================================"
OUTPUT=$($C_TESTER $FILE --analyzer 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    echo "$OUTPUT" | grep -E "^\[ERROR\]|\[OK\]" | head -5
    echo "$OUTPUT" | grep -E "Fix:" | head -3
else
    echo "$OUTPUT" | grep -E "^\[OK\]|\[COMPILE"
fi
echo ""

# --valgrind
echo "========================================"
echo "[3] --VALGRIND (deep mem)"
echo "========================================"
OUTPUT=$($C_TESTER $FILE --valgrind 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    echo "$OUTPUT" | grep -E "^\[ERROR\]|\[OK\]" | head -5
    echo "$OUTPUT" | grep -E "Fix:" | head -3
else
    echo "$OUTPUT" | grep -E "^\[OK\]|\[COMPILE"
fi
echo ""

# Summary
echo "========================================"
echo "  SUMMARY"
echo "========================================"

DETECTION=""
for FLAG in "" "--analyzer" "--valgrind"; do
    OUTPUT=$($C_TESTER $FILE $FLAG 2>&1)
    if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
        [ -z "$FLAG" ] && DETECTION="$DETECTION DEFAULT"
        [ "$FLAG" = "--analyzer" ] && DETECTION="$DETECTION --analyzer"
        [ "$FLAG" = "--valgrind" ] && DETECTION="$DETECTION --valgrind"
    fi
done

if [ -n "$DETECTION" ]; then
    echo "DETECTED by: $DETECTION"
else
    echo "No errors detected (all clean)"
fi

echo ""
echo "Done!"