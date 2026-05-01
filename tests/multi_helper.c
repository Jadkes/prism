/*
 * multi_helper.c - Helper module for multi-file test
 *
 * Purpose: Contains a function with a buffer overflow bug
 *          for testing multi-file error detection.
 */

/*
 * process_data - Process input data and copy to buffer
 *
 * WHY: Intentionally has a stack buffer overflow bug for testing.
 *      The buffer is only 10 bytes but we copy up to 20 bytes.
 *
 * @param input - Input string to process
 * @return Pointer to processed data, or NULL on error
 */
char *process_data(const char *input)
{
    char buffer[10];  /* Intentional small stack buffer */

    /* Bug: copies up to 20 bytes into 10-byte buffer */
    for (int i = 0; i < 20; i++) {
        buffer[i] = input[i];  /* Overflow: writes beyond stack buffer */
    }
    buffer[9] = '\0';  /* Ensure null termination */

    return buffer;  /* Bug: returning pointer to stack memory */
}
