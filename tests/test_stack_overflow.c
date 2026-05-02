/*
 * test_stack_overflow.c - Test: stack overflow via infinite recursion
 *
 * Purpose: Verify c_tester detects stack overflow from infinite recursion.
 * Pattern: "stack-overflow" -> ERR_STACK_OVERFLOW
 * Expected: "[ERROR] Stack Overflow" with fix suggestion.
 */
#include <stdlib.h>

void infinite_recursion(void)
{
    /* No base case - will cause stack overflow */
    infinite_recursion();
}

int main(void)
{
    infinite_recursion();
    return 0;
}
