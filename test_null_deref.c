/*
 * test_null_deref.c - Bug 5: Null pointer dereference
 */
#include <stdio.h>

int main(void)
{
    int *ptr = NULL;
    *ptr = 42;
    return 0;
}