/*
 * test_null_deref_read.c - Test: load of null pointer
 *
 * Purpose: Verify c_tester detects reading from a NULL pointer.
 * Pattern: "load of null pointer" -> ERR_NULL_DEREF
 * Expected: "[ERROR] NULL Pointer Dereference" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int *p = NULL;
    int value = *p;  /* Reading from NULL pointer */
    (void)value;
    return 0;
}
