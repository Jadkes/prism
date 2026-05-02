/*
 * test_valgrind_definite_leak.c - Test: Valgrind definite memory leak
 *
 * Purpose: Verify c_tester detects definite memory leaks.
 * Pattern: "definitely lost" -> ERR_MEMORY_LEAK
 * Expected: "[ERROR] Memory Leak (Definite)" with fix suggestion.
 * Note: Run with valgrind --tool=memcheck ./test_valgrind_definite_leak
 */
#include <stdlib.h>

int main(void)
{
    /* Allocate and lose the pointer - definite leak */
    malloc(100);

    return 0;
}
