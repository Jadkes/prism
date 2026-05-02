/*
 * test_free_nonheap_object.c - Test: free on non-heap object
 *
 * Purpose: Verify c_tester detects calling free() on stack address.
 * Pattern: "free-nonheap" or "free-nonheap-object" -> ERR_UNKNOWN
 * Expected: "[ERROR] Invalid Free" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int stack_var = 42;
    int *p = &stack_var;

    /* Trying to free a stack address - invalid */
    free(p);

    return 0;
}
