/*
 * test_edge_empty_output.c - Edge case: no output
 *
 * Purpose: Verify c_tester handles programs that produce no output.
 *          Program outputs nothing to stdout (empty stdout).
 *
 * Expected: c_tester completes with "No errors detected".
 */
#include <stdio.h>

int main(void)
{
    /* Intentionally produce no output */
    return 0;
}
