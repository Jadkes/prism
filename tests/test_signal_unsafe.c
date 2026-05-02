/*
 * test_signal_unsafe.c - Test: signal-unsafe call in signal handler
 *
 * Purpose: Verify c_tester detects async-signal-unsafe calls in handlers.
 * Pattern: "signal-unsafe-call" -> ERR_DATA_RACE
 * Expected: "[ERROR] Signal-Unsafe Call" with fix suggestion.
 * Note: Requires -fsanitize=thread or careful signal testing.
 */
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

void unsafe_handler(int sig)
{
    (void)sig;
    /* printf is not async-signal-safe */
    printf("Signal received!\n");
}

int main(void)
{
    signal(SIGUSR1, unsafe_handler);

    /* Raise signal to trigger handler */
    raise(SIGUSR1);

    return 0;
}
