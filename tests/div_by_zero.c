/*
 * div_by_zero.c - Test: division by zero
 *
 * Purpose: Verify c_tester detects division by zero.
 * Expected: "[ERROR] Division by Zero" with fix suggestion.
 */
int main(void)
{
    int x = 1 / 0;
    return x;
}
