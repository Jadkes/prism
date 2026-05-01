/*
 * c_tester.h - Header file for c_tester, a simple C error detection tool
 *
 * Purpose: Provides error detection, memory leak detection, and debugging
 *          capabilities that are simpler than GDB and catch more errors
 *          than standard compilation.
 *
 * Design: Wraps GCC compilation with ASan/UBSan sanitizers, falls back to
 *         signal-based detection if sanitizers are unavailable. Parses error
 *         output and provides human-readable fix suggestions.
 *
 * Thread-safety: Single-threaded tool, no concurrency concerns.
 */

#ifndef C_TESTER_H
#define C_TESTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

/* Constants */
#define MAX_LINE_LEN        2048
#define MAX_OUTPUT_SIZE     (64 * 1024)
#define MAX_SOURCE_LINE     512
#define MAX_PATH_LEN        4096
#define EXIT_CLEAN          0
#define EXIT_ERRORS_FOUND   1
#define EXIT_COMPILE_FAIL   2
#define EXIT_FILE_NOT_FOUND 3
#define EXIT_USAGE_ERROR    4
#define DEFAULT_TIMEOUT_SEC 30

/* Error types detected by c_tester */
typedef enum {
    ERR_NULL_DEREF,
    ERR_BUFFER_OVERFLOW,
    ERR_USE_AFTER_FREE,
    ERR_MEMORY_LEAK,
    ERR_INT_OVERFLOW,
    ERR_DIV_BY_ZERO,
    ERR_UNINIT_VAR,
    ERR_STACK_OVERFLOW,
    ERR_SEGV,
    ERR_ABORT,
    ERR_UNKNOWN,
    ERR_NONE
} ErrorType;

/* Represents a detected error with context and fix suggestion */
typedef struct {
    ErrorType type;
    char title[256];
    char fix_suggestion[1024];
    char source_file[MAX_PATH_LEN];
    int source_line;
    int severity;
    bool has_source;
} DetectedError;

/* Result of compiling and running a test file */
typedef struct {
    bool compilation_success;
    int exit_code;
    int signal_received;
    char compiler_output[MAX_OUTPUT_SIZE];
    char runtime_output[MAX_OUTPUT_SIZE];
    char sanitizer_output[MAX_OUTPUT_SIZE];
    long execution_time_ms;
} TestResult;

/* Pattern to match in error output for classification */
typedef struct {
    const char *pattern;
    const char *title;
    const char *fix;
    ErrorType type;
    int severity;
} ErrorPattern;

/* ANSI color codes for terminal output */
typedef struct {
    const char *red;
    const char *green;
    const char *yellow;
    const char *blue;
    const char *magenta;
    const char *cyan;
    const char *bold;
    const char *reset;
} ColorCodes;

/* Initialize color codes based on whether colors are enabled */
void init_colors(ColorCodes *colors, bool use_colors);

/* Print formatted message with color */
void print_colored(const ColorCodes *colors, const char *color, const char *fmt, ...);

/* Print banner showing file being tested */
void print_banner(const ColorCodes *colors, const char *filename);

/* Print summary of test results */
void print_summary(const ColorCodes *colors, const TestResult *result, int error_count, const DetectedError *errors, const char *source_file);

/*
 * compile_with_sanitizers - Compile source with ASan and UBSan
 *
 * WHY: Sanitizers catch runtime errors at execution time that compilers
 *      miss. ASan catches memory errors, UBSan catches undefined behavior.
 *
 * @param source - Path to C source file
 * @param binary - Output binary path
 * @param output - Buffer for compiler output
 * @param output_size - Size of output buffer
 * @return 0 on success, non-zero on failure
 */
int compile_with_sanitizers(const char *source, const char *binary,
                            char *output, size_t output_size);

/*
 * compile_fallback - Compile without sanitizers as fallback
 *
 * WHY: Some systems lack sanitizer libraries. Fallback to basic
 *      compilation with -Wall -Wextra for basic error detection.
 */
int compile_fallback(const char *source, const char *binary,
                    char *output, size_t output_size);

/*
 * compile_for_warnings - Compile with warning flags only (no sanitizers)
 *
 * WHY: Sanitizers suppress certain warnings (e.g., strict-aliasing).
 *      A separate warning-only pass catches these at -O2 optimization.
 */
int compile_for_warnings(const char *source, char *output, size_t output_size);

/*
 * run_binary - Execute compiled binary with timeout
 *
 * WHY: Need to run test with timeout to catch infinite loops
 *      and capture both stdout and stderr for analysis.
 *
 * @param binary - Path to binary to execute
 * @param args - Arguments to pass (or NULL)
 * @param output - Buffer for runtime output
 * @param output_size - Size of output buffer
 * @param error_output - Buffer for stderr/sanitizer output
 * @param error_size - Size of error buffer
 * @param timeout_sec - Maximum execution time in seconds
 * @param child_pid - Output: PID of child process
 * @return Exit code of binary, or -1 on failure
 */
int run_binary(const char *binary, const char *args,
               char *output, size_t output_size,
               char *error_output, size_t error_size,
               int timeout_sec, pid_t *child_pid);

/*
 * classify_error - Determine error type from error line
 *
 * WHY: Sanitizer output contains specific keywords that identify
 *      the type of error (e.g., "heap-buffer-overflow").
 */
ErrorType classify_error(const char *error_line);

/*
 * parse_sanitizer_errors - Extract errors from sanitizer output
 *
 * WHY: Sanitizer output is multi-line with specific format.
 *      Parse it to extract structured error information.
 *
 * @param error_output - Raw sanitizer output
 * @param errors - Array to fill with detected errors
 * @param max_errors - Maximum number of errors to detect
 * @return Number of errors found
 */
int parse_sanitizer_errors(const char *error_output,
                           DetectedError *errors, int max_errors);

/*
 * parse_signal_errors - Detect errors from signal termination
 *
 * WHY: When a program crashes with a signal (SIGSEGV, SIGABRT, etc.),
 *      we can classify the error based on the signal received.
 */
int parse_signal_errors(const char *error_output, int exit_code,
                        int signal, DetectedError *errors, int max_errors);

/*
 * generate_fix_suggestion - Create fix suggestion for detected error
 *
 * WHY: The tool should be helpful by suggesting how to fix
 *      each type of error, not just report it.
 */
void generate_fix_suggestion(DetectedError *error);

/* Get human-readable name for error type */
const char *get_error_name(ErrorType type);

/*
 * get_source_line - Read a specific line from source file
 *
 * WHY: When reporting errors, showing the offending line of code
 *      helps the user understand and fix the problem.
 */
int get_source_line(const char *source_file, int line_number,
                    char *buffer, size_t buffer_size);

/* Check if a file exists at the given path */
bool file_exists(const char *path);

/* Check if path has a .c extension */
bool is_c_file(const char *path);

/*
 * has_main_function - Check if source file contains main()
 *
 * WHY: Only files with main() can be compiled and executed.
 *      Skip test files that are meant to be #included or
 *      compiled as part of a larger program.
 */
bool has_main_function(const char *source_file);

/*
 * generate_temp_path - Create a temporary file path
 *
 * WHY: Need to create temporary binary files for each test
 *      without conflicting with existing files or other tests.
 */
char *generate_temp_path(const char *prefix, char *buffer, size_t buffer_size);

int cleanup_binary(const char *binary);

/* Check if haystack contains needle (case-sensitive) */
bool string_contains(const char *haystack, const char *needle);

/* Check if str starts with prefix */
bool string_starts_with(const char *str, const char *prefix);

/* Remove leading and trailing whitespace */
void trim_whitespace(char *str);

/* Get file size in bytes, or -1 on error */
long get_file_size(const char *path);

/*
 * get_execution_time - Calculate elapsed time in milliseconds
 *
 * WHY: Need to track how long a test took to execute,
 *      and enforce timeout for infinite loops.
 */
void get_execution_time(struct timespec *start, struct timespec *end, long *ms);

#endif /* C_TESTER_H */
