/*
 * test_valgrind_invalid_write.c - Test: Valgrind invalid write
 *
 * Purpose: Verify c_tester detects invalid writes under Valgrind.
 * Pattern: "Invalid write" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Invalid Write" with fix suggestion.
 * Note: Run with valgrind --tool=memcheck ./test_valgrind_invalid_write
 */
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *buf = malloc(5);
    if (!buf) return 1;

    /* Invalid write - past allocated memory */
    buf[5] = 'X';  /* One byte past end */

    free(buf);
    return 0;
}
