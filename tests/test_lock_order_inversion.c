/*
 * test_lock_order_inversion.c - Test: ThreadSanitizer lock order inversion
 *
 * Purpose: Verify c_tester detects inconsistent lock ordering (deadlock risk).
 * Pattern: "lock-order-inversion" -> ERR_DATA_RACE
 * Expected: "[ERROR] Lock Order Inversion" with fix suggestion.
 * Note: Requires -fsanitize=thread and pthread.
 */
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

void *thread_one(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&lock_a);
    usleep(100);  /* Ensure thread_two grabs lock_b */
    pthread_mutex_lock(&lock_b);
    pthread_mutex_unlock(&lock_b);
    pthread_mutex_unlock(&lock_a);
    return NULL;
}

void *thread_two(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&lock_b);
    usleep(100);  /* Ensure thread_one grabs lock_a */
    pthread_mutex_lock(&lock_a);
    pthread_mutex_unlock(&lock_a);
    pthread_mutex_unlock(&lock_b);
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread_one, NULL);
    pthread_create(&t2, NULL, thread_two, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&lock_a);
    pthread_mutex_destroy(&lock_b);
    return 0;
}
