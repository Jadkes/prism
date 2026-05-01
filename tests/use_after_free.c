/*
 * use_after_free.c - Test: use after free
 *
 * Purpose: Verify c_tester detects accessing freed memory.
 * Expected: "[ERROR] Use After Free" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int *p = malloc(sizeof(int));
    free(p);
    *p = 1;
    return 0;
}
