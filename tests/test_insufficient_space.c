/*
 * test_insufficient_space.c - Test: insufficient space (buffer overflow)
 *
 * Purpose: Verify c_tester detects buffer overflow via memcpy.
 * Pattern: "insufficient space" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Buffer Overflow" with fix suggestion.
 */
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *dst = malloc(5);
    char *src = "this is too long";

    if (!dst) return 1;

    /* memcpy with insufficient destination space */
    memcpy(dst, src, strlen(src) + 1);

    free(dst);
    return 0;
}
