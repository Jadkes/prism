/*
 * test_valgrind_uninit_conditional.c - Test: Valgrind uninitialized conditional
 *
 * Purpose: Verify c_tester detects uninitialized values in conditionals.
 * Pattern: "Conditional jump or move depends on uninitialised" -> ERR_UNINIT_VAR
 * Expected: "[ERROR] Uninitialized Conditional" with fix suggestion.
 * Note: Run with valgrind --tool=memcheck ./test_valgrind_uninit_conditional
 */
#include <stdlib.h>

int main(void)
{
    int x;  /* Uninitialized */

    /* Use uninitialized value in conditional */
    if (x > 0) {
        return 1;
    }

    return 0;
}
