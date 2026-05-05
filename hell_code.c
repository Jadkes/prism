/*
 * hell_code.c - A file full of every bug imaginable
 * Purpose: Test c_tester's error detection capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUG_MACRO(x) ((x) / 0)  /* Division by zero via macro */

/* Bug 1: Stack buffer overflow */
void bug_stack_overflow(void)
{
    char small_buf[8];
    strcpy(small_buf, "This string is way too long for the buffer!");  /* overflow */
}

/* Bug 2: Heap buffer overflow */
void bug_heap_overflow(void)
{
    char *buf = malloc(8);
    strcpy(buf, "Writing way more than 8 bytes to heap buffer!!!");  /* overflow */
    free(buf);
}

/* Bug 3: Use after free */
void bug_use_after_free(void)
{
    char *buf = malloc(32);
    strcpy(buf, "hello");
    free(buf);
    printf("%s\n", buf);  /* use after free */
}

/* Bug 4: Memory leak */
void bug_memory_leak(void)
{
    char *buf = malloc(128);
    strcpy(buf, "leaked!");
    /* forgot to free(buf) */
}

/* Bug 5: Null pointer dereference */
void bug_null_deref(void)
{
    int *ptr = NULL;
    *ptr = 42;  /* dereference null */
}

/* Bug 6: Division by zero */
int bug_div_by_zero(int x)
{
    return x / 0;  /* literal zero */
}

/* Bug 7: Uninitialized variable */
int bug_uninit_var(void)
{
    int val;
    return val + 1;  /* uninitialized */
}

/* Bug 8: Double free */
void bug_double_free(void)
{
    char *buf = malloc(32);
    strcpy(buf, "test");
    free(buf);
    free(buf);  /* double free */
}

/* Bug 9: Integer overflow */
int bug_int_overflow(void)
{
    int val = 2147483647;
    return val + 1;  /* overflow */
}

/* Bug 10: Infinite recursion (stack overflow) */
int bug_infinite_recursion(int n)
{
    return bug_infinite_recursion(n + 1);  /* no base case */
}

/* Bug 11: Negative shift */
int bug_shift_overflow(void)
{
    int x = 1;
    return x << -1;  /* negative shift */
}

/* Bug 12: Stack buffer underflow */
void bug_stack_underflow(void)
{
    char buf[16] = "test";
    printf("%c\n", buf[-5]);  /* negative index */
}

/* Bug 13: Invalid free (not heap) */
void bug_free_stack(void)
{
    char buf[64];
    free(buf);  /* free stack memory */
}

/* Bug 14: Uninitialized pointer */
void bug_uninit_ptr(void)
{
    char *ptr;  /* uninitialized */
    strcpy(ptr, "crash");  /* write to garbage pointer */
}

/* Bug 15: sprintf overflow */
void bug_sprintf_overflow(void)
{
    char buf[8];
    sprintf(buf, "This string is way too long for the buffer");  /* overflow */
}

/* Main: run each bug */
int main(int argc, char *argv[])
{
    int choice = 0;

    if (argc > 1) {
        choice = atoi(argv[1]);
    }

    switch (choice) {
        case 1:  bug_stack_overflow();      break;
        case 2:  bug_heap_overflow();       break;
        case 3:  bug_use_after_free();     break;
        case 4:  bug_memory_leak();        break;
        case 5:  bug_null_deref();       break;
        case 6:  return bug_div_by_zero(10);
        case 7:  return bug_uninit_var();
        case 8:  bug_double_free();      break;
        case 9:  return bug_int_overflow();
        case 10: bug_infinite_recursion(0);
        case 11: return bug_shift_overflow();
        case 12: bug_stack_underflow();   break;
        case 13: bug_free_stack();       break;
        case 14: bug_uninit_ptr();       break;
        case 15: bug_sprintf_overflow(); break;
        default:
            printf("Usage: %s <bug_number 1-15>\n", argv[0]);
            printf("Testing bug 1 (stack overflow)...\n");
            bug_stack_overflow();
    }

    return 0;
}