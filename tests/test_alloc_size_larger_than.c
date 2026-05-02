/*
 * test_alloc_size_larger_than.c - Test: alloc-size-larger-than
 *
 * Purpose: Verify c_tester detects excessive allocation requests.
 * Pattern: "alloc-size-larger-than" -> ERR_INT_OVERFLOW
 * Expected: "[ERROR] Excessive Allocation" with fix suggestion.
 * Note: This requires -fsanitize=address with UBSan to trigger.
 */
#include <stdlib.h>
#include <limits.h>

int main(void)
{
    /* Request allocation larger than maximum object size */
    /* This triggers "alloc-size-larger-than" in ASan */
    size_t huge_size = (size_t)-1;
    void *p = malloc(huge_size);

    if (p) {
        free(p);
    }

    return 0;
}
