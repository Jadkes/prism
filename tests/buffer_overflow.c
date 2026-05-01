/*
 * buffer_overflow.c - Test: stack buffer overflow
 *
 * Purpose: Verify c_tester detects writing past array bounds.
 * Expected: "[ERROR] Stack Buffer Overflow" with fix suggestion.
 */
#include <string.h>

int main(void)
{
    char buf[5];
    strcpy(buf, "this is way too long");
    return 0;
}
