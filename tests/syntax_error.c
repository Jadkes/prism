/*
 * syntax_error.c - Test: compilation failure
 *
 * Purpose: Verify c_tester reports compile errors clearly.
 * Expected: "[COMPILE ERROR]" with compiler output.
 */
int main(void)
{
    missing_semicolon
    return 0;
}
