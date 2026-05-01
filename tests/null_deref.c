/*
 * null_deref.c - Test: NULL pointer dereference
 *
 * Purpose: Verify c_tester detects writing to a NULL pointer.
 * Expected: "[ERROR] NULL Pointer Dereference" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int *p = NULL;
    *p = 42;
    return 0;
}
