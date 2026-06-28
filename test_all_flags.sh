#!/bin/bash
#
# test_all_flags.sh - Test prism with all available flags
#
# Runs prism with each flag on hell_code.c and shows the results

C_TESTER="./prism"
TEST_FILE="hell_code.c"

echo "========================================"
echo "  prism - Full Flag Test Suite"
echo "========================================"
echo ""

# Check prism exists
if [ ! -x "$C_TESTER" ]; then
    echo "Error: prism not found or not executable"
    exit 1
fi

# Check test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo "Error: $TEST_FILE not found"
    exit 1
fi

echo "Test file: $TEST_FILE"
echo ""

# Array to store results
declare -A results

# Test 1: Default (ASan + UBSan)
echo "----------------------------------------"
echo "[TEST 1] Default (ASan + UBSan)"
echo "----------------------------------------"
OUTPUT=$($C_TESTER $TEST_FILE 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    results[default]="DETECTED"
    echo "$OUTPUT" | grep "\[ERROR\]" | head -3
else
    results[default]="CLEAN"
    echo "[OK] No errors detected"
fi
echo ""

# Test 2: --analyzer (GCC static)
echo "----------------------------------------"
echo "[TEST 2] --analyzer (GCC static analyzer)"
echo "----------------------------------------"
OUTPUT=$($C_TESTER $TEST_FILE --analyzer 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    results[analyzer]="DETECTED"
    echo "$OUTPUT" | grep "\[ERROR\]" | head -3
else
    results[analyzer]="CLEAN"
    echo "[OK] No errors detected"
fi
echo ""

# Test 3: --valgrind
echo "----------------------------------------"
echo "[TEST 3] --valgrind (Valgrind memory)"
echo "----------------------------------------"
OUTPUT=$($C_TESTER $TEST_FILE --valgrind 2>&1)
if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
    results[valgrind]="DETECTED"
    echo "$OUTPUT" | grep "\[ERROR\]" | head -3
else
    results[valgrind]="CLEAN"
    echo "[OK] No errors detected"
fi
echo ""

# Test 4: Test individual bug files with --analyzer
echo "========================================"
echo "  Individual Bug Tests (--analyzer)"
echo "========================================"
echo ""

for bug_file in test_null_deref.c test_doublefree.c test_uaf.c test_leak.c; do
    if [ -f "$bug_file" ]; then
        echo "--- $bug_file ---"
        OUTPUT=$($C_TESTER $bug_file --analyzer 2>&1)
        if echo "$OUTPUT" | grep -q "\[ERROR\]"; then
            echo "$OUTPUT" | grep "\[ERROR\]" | head -1
        else
            echo "[OK] Clean"
        fi
    fi
done
echo ""

# Summary
echo "========================================"
echo "  SUMMARY"
echo "========================================"
echo ""
printf "%-12s | %s\n" "Flag" "Result"
echo "-------------+--------"
printf "%-12s | %s\n" "default" "${results[default]}"
printf "%-12s | %s\n" "--analyzer" "${results[analyzer]}"
printf "%-12s | %s\n" "--valgrind" "${results[valgrind]}"
echo ""
echo "Done."