/*
 * uninit_var.c - Test: uninitialized variable
 *
 * Purpose: Verify c_tester detects use of uninitialized values.
 * Expected: "[ERROR] Uninitialized Variable" with fix suggestion.
 */
#include <stdio.h>

int main(void)
{
    int x;
    printf("%d\n", x);
    return 0;
}
