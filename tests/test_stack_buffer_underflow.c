/*
 * test_stack_buffer_underflow.c - Test: stack buffer underflow
 *
 * Purpose: Verify c_tester detects accessing before stack buffer start.
 * Pattern: "stack-buffer-underflow" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Buffer Underflow" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    char buf[10];
    /* Access before start of buffer */
    char value = buf[-1];
    (void)value;
    return 0;
}
