/*
 * test_null_pointer_passed.c - Test: null pointer passed to function
 *
 * Purpose: Verify c_tester detects NULL passed to function requiring valid pointer.
 * Pattern: "null pointer passed" -> ERR_NULL_DEREF
 * Expected: "[ERROR] NULL Pointer Passed to Function" with fix suggestion.
 */
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *str = NULL;
    /* strlen requires valid pointer - passing NULL triggers error */
    size_t len = strlen(str);
    (void)len;
    return 0;
}
