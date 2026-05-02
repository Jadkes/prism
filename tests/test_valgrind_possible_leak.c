/*
 * test_valgrind_possible_leak.c - Test: Valgrind possible memory leak
 *
 * Purpose: Verify c_tester detects possible memory leaks.
 * Pattern: "possibly lost" -> ERR_MEMORY_LEAK
 * Expected: "[ERROR] Memory Leak (Possible)" with fix suggestion.
 * Note: Run with valgrind --tool=memcheck ./test_valgrind_possible_leak
 */
#include <stdlib.h>

int main(void)
{
    /* Allocate and move pointer - possible leak */
    char *p = malloc(100);
    char *q = p + 50;  /* Pointer arithmetic moves original reference */

    /* Only free part of the allocation */
    free(q);  /* Actually invalid, but demonstrates possible loss */

    return 0;
}
