/*
 * clean.c - Test: valid program with no errors
 *
 * Purpose: Verify c_tester reports clean run for correct code.
 * Expected: "[OK] No errors detected" with exit code 0.
 */
#include <stdio.h>

int main(void)
{
    printf("hello\n");
    return 0;
}
