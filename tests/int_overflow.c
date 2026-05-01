/*
 * int_overflow.c - Test: signed integer overflow
 *
 * Purpose: Verify c_tester detects integer overflow.
 * Expected: "[ERROR] Integer Overflow" with fix suggestion.
 */
#include <limits.h>

int main(void)
{
    int x = INT_MAX;
    x += 1;
    return x;
}
