/*
 * multi_main.c - Main file for multi-file test
 *
 * Purpose: Calls helper function from multi_helper.c
 *          to test multi-file compilation and error detection.
 */

/* Declare helper function from multi_helper.c */
char *process_data(const char *input);

/*
 * main - Entry point that calls helper with overflow input
 *
 * WHY: Calls process_data() with input that triggers
 *      the buffer overflow bug in multi_helper.c.
 */
int main(void)
{
    /* Call helper with string longer than 10 bytes */
    char *result = process_data("This is a long string that overflows");

    if (result) {
        return 0;
    }
    return 1;
}
