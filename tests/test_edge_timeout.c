/*
 * test_edge_timeout.c - Edge case: infinite loop with timeout
 *
 * Purpose: Verify c_tester detects and kills hung processes.
 *          Similar to infinite_loop.c but as an edge case test.
 *
 * Expected: c_tester times out and reports timeout.
 */
int main(void)
{
    /* Infinite loop - c_tester should kill within timeout */
    while (1)
        ;
    return 0;
}
