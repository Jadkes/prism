/*
 * test_valgrind_invalid_read.c - Test: Valgrind invalid read
 *
 * Purpose: Verify c_tester detects invalid reads under Valgrind.
 * Pattern: "Invalid read" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Invalid Read" with fix suggestion.
 * Note: Run with valgrind --tool=memcheck ./test_valgrind_invalid_read
 */
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *buf = malloc(10);
    if (!buf) return 1;

    /* Valid write */
    strcpy(buf, "hello");

    /* Invalid read - past allocated memory */
    char value = buf[10];  /* One byte past end */
    (void)value;

    free(buf);
    return 0;
}
