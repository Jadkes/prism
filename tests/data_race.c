/*
 * data_race.c - Test: trigger a data race detected by TSan
 *
 * Purpose: Verify c_tester --tsan detects unsynchronized access.
 * Expected: "Data Race" error when run with --tsan flag.
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int shared_counter = 0;

void *increment(void *arg)
{
    (void)arg;
    /* No lock protecting shared_counter */
    shared_counter++;
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter=%d\n", shared_counter);
    return 0;
}
