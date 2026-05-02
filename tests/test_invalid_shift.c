/*
 * test_invalid_shift.c - Test: invalid shift exponent
 *
 * Purpose: Verify c_tester detects invalid shift operations.
 * Pattern: "shift exponent" -> ERR_INT_OVERFLOW
 * Expected: "[ERROR] Invalid Shift" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int value = 1;
    /* Shift by negative value - invalid */
    int result = value << -1;
    (void)result;

    /* Shift by >= type width (32 for int) - invalid */
    result = value << 32;
    (void)result;

    return 0;
}
