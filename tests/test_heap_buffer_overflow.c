/*
 * test_heap_buffer_overflow.c - Test: heap buffer overflow
 *
 * Purpose: Verify c_tester detects writing past heap-allocated buffer.
 * Pattern: "heap-buffer-overflow" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Buffer Overflow" with fix suggestion.
 */
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *buf = malloc(5);
    if (!buf) return 1;

    /* Write past allocated buffer */
    strcpy(buf, "this is way too long for 5 bytes");

    free(buf);
    return 0;
}
