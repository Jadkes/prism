/*
 * test_out_of_bounds.c - Test: array out of bounds
 *
 * Purpose: Verify c_tester detects array index outside valid range.
 * Pattern: "out of bounds" -> ERR_BUFFER_OVERFLOW
 * Expected: "[ERROR] Array Out of Bounds" with fix suggestion.
 */
#include <stdlib.h>

int main(void)
{
    int arr[5] = {0, 1, 2, 3, 4};

    /* Access far beyond array bounds */
    int value = arr[100];
    (void)value;

    return 0;
}
