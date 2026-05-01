/*
 * valgrind_uninit.c - Test: uninitialized variable under Valgrind
 *
 * Purpose: Verify c_tester --valgrind detects uninitialized value usage.
 * Expected: Valgrind reports "Use of uninitialised value" or similar.
 */
#include <stdio.h>

int main(void)
{
    int x;
    if (x > 0)
        printf("positive\n");
    return 0;
}
