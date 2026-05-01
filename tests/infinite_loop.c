/*
 * infinite_loop.c - Test: infinite loop (timeout)
 *
 * Purpose: Verify c_tester kills hung processes within timeout.
 * Expected: timeout message within 30 seconds.
 */
int main(void)
{
    while (1)
        ;
    return 0;
}
