/*
 * test_edge_long_path.c - Edge case: long path handling
 *
 * Purpose: Verify c_tester handles long filenames/paths correctly.
 *          MAX_PATH_LEN is 4096 in c_tester.h.
 *
 * Expected: c_tester processes file without buffer overflow.
 *           Test is run via symlink with long name (see Makefile).
 */
#include <stdio.h>

int main(void)
{
    printf("long_path_test\n");
    return 0;
}
