/*
 * memory_leak.c - Test: memory leak (malloc without free)
 *
 * Purpose: Verify c_tester detects unfreed allocations.
 * Expected: "[ERROR] Memory Leak" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    malloc(100);
    return 0;
}
