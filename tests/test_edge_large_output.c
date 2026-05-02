/*
 * test_edge_large_output.c - Edge case: output exceeds MAX_OUTPUT_SIZE
 *
 * Purpose: Verify c_tester handles output larger than 64KB buffer.
 *          MAX_OUTPUT_SIZE is 64*1024 = 65536 bytes.
 *          This program prints ~100KB to test truncation behavior.
 *
 * Expected: c_tester captures what it can without crashing.
 *           Output may be truncated but should not cause buffer overflow.
 */
#include <stdio.h>

int main(void)
{
    int i;
    char buf[1024];
    /* Fill buffer with printable chars, repeat to exceed 64KB */
    for (i = 0; i < 1023; i++)
        buf[i] = 'A' + (i % 26);
    buf[1023] = '\n';

    /* 100 iterations * 1024 bytes = ~100KB, exceeds MAX_OUTPUT_SIZE (64KB) */
    for (i = 0; i < 100; i++) {
        fwrite(buf, 1, 1024, stdout);
    }
    return 0;
}
