/*
 * test_edge_rapid_alloc.c - Edge case: rapid memory allocation
 *
 * Purpose: Verify c_tester handles programs with rapid successive
 *          malloc() calls without leaking or crashing.
 *
 * Expected: c_tester completes without issues (or detects if
 *           there's a real memory error).
 */
#include <stdlib.h>

int main(void)
{
    int i;
    void *ptrs[1000];

    /* Rapid allocation and deallocation */
    for (i = 0; i < 1000; i++) {
        ptrs[i] = malloc(1024);  /* 1KB each */
        if (ptrs[i])
            free(ptrs[i]);
    }
    return 0;
}
