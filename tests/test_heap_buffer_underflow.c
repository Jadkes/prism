/*
 * test_heap_buffer_underflow.c - Test: heap buffer underflow
 *
 * Purpose: Verify c_tester detects accessing before heap buffer start.
 * Pattern: "heap-buffer-underflow" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Buffer Underflow" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    char *buf = malloc(10);
    if (!buf) return 1;

    /* Access before start of allocated memory */
    char value = buf[-1];
    (void)value;

    free(buf);
    return 0;
}
