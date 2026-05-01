/*
 * c_tester.c - Core engine for c_tester, a simple C error detection tool
 *
 * Purpose: Compiles C source files with GCC sanitizers (ASan/UBSan),
 *          runs them with timeout, captures output, and provides the
 *          foundation for error detection and fix suggestions.
 *
 * Design: Uses popen() for compilation output capture, fork()/execvp()/select()
 *         for binary execution with timeout. Falls back to non-sanitizer
 *         compilation when sanitizer libraries are unavailable.
 *
 * Thread-safety: Single-threaded tool, no concurrency concerns.
 */

#include "c_tester.h"

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
                            char *output, size_t output_size)
{
    char cmd[MAX_PATH_LEN * 2 + 128];
    FILE *pipe;
    size_t bytes_read;

    snprintf(cmd, sizeof(cmd),
             "gcc -fsanitize=address,undefined -g -fno-omit-frame-pointer "
             "-Wuninitialized -o '%s' '%s' 2>&1", binary, source);

    pipe = popen(cmd, "r");
    if (!pipe) {
        snprintf(output, output_size, "Failed to start compiler");
        return -1;
    }

    bytes_read = fread(output, 1, output_size - 1, pipe);
    output[bytes_read] = '\0';

    return pclose(pipe);
}

/*
 * compile_fallback - Compile without sanitizers as fallback
 *
 * WHY: Some systems lack sanitizer libraries. Fallback to basic
 *      compilation with -Wall -Wextra for basic error detection.
 */
int compile_fallback(const char *source, const char *binary,
                     char *output, size_t output_size)
{
    char cmd[MAX_PATH_LEN * 2 + 64];
    FILE *pipe;
    size_t bytes_read;

    snprintf(cmd, sizeof(cmd),
             "gcc -Wall -Wextra -g -o '%s' '%s' 2>&1", binary, source);

    pipe = popen(cmd, "r");
    if (!pipe) {
        snprintf(output, output_size, "Failed to start compiler");
        return -1;
    }

    bytes_read = fread(output, 1, output_size - 1, pipe);
    output[bytes_read] = '\0';

    return pclose(pipe);
}

/*
 * run_binary - Execute compiled binary with timeout
 *
 * WHY: Need to run test with timeout to catch infinite loops
 *      and capture both stdout and stderr for analysis.
 */
int run_binary(const char *binary, const char *args,
               char *output, size_t output_size,
               char *error_output, size_t error_size,
               int timeout_sec, pid_t *child_pid)
{
    int stdout_pipe[2], stderr_pipe[2];
    pid_t pid;
    char *argv[3];
    int max_fd;
    fd_set read_fds;
    struct timeval tv;
    int stdout_eof = 0, stderr_eof = 0;
    size_t out_pos = 0, err_pos = 0;
    int status;

    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0)
        return -1;

    pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        argv[0] = (char *)binary;
        argv[1] = (char *)args;
        argv[2] = NULL;

        execvp(binary, argv);
        _exit(127);
    }

    /* Parent process */
    if (child_pid)
        *child_pid = pid;

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += timeout_sec;

    while (!stdout_eof || !stderr_eof) {
        FD_ZERO(&read_fds);
        max_fd = 0;

        if (!stdout_eof) {
            FD_SET(stdout_pipe[0], &read_fds);
            if (stdout_pipe[0] > max_fd)
                max_fd = stdout_pipe[0];
        }
        if (!stderr_eof) {
            FD_SET(stderr_pipe[0], &read_fds);
            if (stderr_pipe[0] > max_fd)
                max_fd = stderr_pipe[0];
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long remaining_ms = (deadline.tv_sec - now.tv_sec) * 1000 +
                            (deadline.tv_nsec - now.tv_nsec) / 1000000;
        if (remaining_ms <= 0) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            if (out_pos < output_size)
                output[out_pos] = '\0';
            if (err_pos < error_size)
                error_output[err_pos] = '\0';
            return -1;
        }

        tv.tv_sec = remaining_ms / 1000;
        tv.tv_usec = (remaining_ms % 1000) * 1000;

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready < 0)
            break;

        if (ready == 0) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            if (out_pos < output_size)
                output[out_pos] = '\0';
            if (err_pos < error_size)
                error_output[err_pos] = '\0';
            return -1;
        }

        if (FD_ISSET(stdout_pipe[0], &read_fds)) {
            ssize_t n = read(stdout_pipe[0], output + out_pos,
                             output_size - out_pos - 1);
            if (n <= 0)
                stdout_eof = 1;
            else
                out_pos += n;
        }
        if (FD_ISSET(stderr_pipe[0], &read_fds)) {
            ssize_t n = read(stderr_pipe[0], error_output + err_pos,
                             error_size - err_pos - 1);
            if (n <= 0)
                stderr_eof = 1;
            else
                err_pos += n;
        }
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (out_pos < output_size)
        output[out_pos] = '\0';
    if (err_pos < error_size)
        error_output[err_pos] = '\0';

    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return -1;
}

/*
 * generate_temp_path - Create a temporary file path
 *
 * WHY: Need to create temporary binary files for each test
 *      without conflicting with existing files or other tests.
 */
char *generate_temp_path(const char *prefix, char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "/tmp/c_tester_%s_XXXXXX", prefix);
    int fd = mkstemp(buffer);
    if (fd >= 0)
        close(fd);
    return buffer;
}

/* Remove compiled binary after run */
int cleanup_binary(const char *binary)
{
    return unlink(binary);
}

/* Check if a file exists at the given path */
bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* Check if path has a .c extension */
bool is_c_file(const char *path)
{
    const char *dot = strrchr(path, '.');
    return dot && string_starts_with(dot, ".c") && dot[2] == '\0';
}

/*
 * has_main_function - Check if source file contains main()
 *
 * WHY: Only files with main() can be compiled and executed.
 */
bool has_main_function(const char *source_file)
{
    FILE *fp;
    char line[MAX_SOURCE_LINE];
    bool found = false;

    fp = fopen(source_file, "r");
    if (!fp)
        return false;

    while (fgets(line, sizeof(line), fp)) {
        if (string_contains(line, "int main") ||
            string_contains(line, "main(")) {
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

/* Check if haystack contains needle (case-sensitive) */
bool string_contains(const char *haystack, const char *needle)
{
    return haystack && needle && strstr(haystack, needle) != NULL;
}

/* Check if str starts with prefix */
bool string_starts_with(const char *str, const char *prefix)
{
    if (!str || !prefix)
        return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* Remove leading and trailing whitespace */
void trim_whitespace(char *str)
{
    char *start, *end;

    if (!str)
        return;

    /* Skip leading whitespace */
    start = str;
    while (*start && (*start == ' ' || *start == '\t' ||
                      *start == '\n' || *start == '\r'))
        start++;

    /* All whitespace */
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }

    /* Move to start */
    if (start != str)
        memmove(str, start, strlen(start) + 1);

    /* Trim trailing whitespace */
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' ||
                         *end == '\n' || *end == '\r'))
        *end-- = '\0';
}

/* Get file size in bytes, or -1 on error */
long get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return -1;
    return st.st_size;
}

/*
 * get_execution_time - Calculate elapsed time in milliseconds
 *
 * WHY: Need to track how long a test took to execute,
 *      and enforce timeout for infinite loops.
 */
void get_execution_time(struct timespec *start, struct timespec *end, long *ms)
{
    *ms = (end->tv_sec - start->tv_sec) * 1000 +
          (end->tv_nsec - start->tv_nsec) / 1000000;
}

/* Error pattern table mapping sanitizer output to error types and fixes */
static const ErrorPattern error_patterns[] = {
    { "heap-buffer-overflow",    "Buffer Overflow",
      "Check array bounds. The buffer was allocated on the heap but you accessed memory beyond its limits. Use bounds checking or increase the buffer size.",
      ERR_BUFFER_OVERFLOW, 2 },
    { "stack-buffer-overflow",   "Stack Buffer Overflow",
      "The local array is too small for the data you're writing. Use dynamic allocation (malloc) for large arrays, or increase the buffer size.",
      ERR_STACK_OVERFLOW, 2 },
    { "heap-use-after-free",     "Use After Free",
      "You accessed memory after calling free(). Set the pointer to NULL after free() and check for NULL before accessing.",
      ERR_USE_AFTER_FREE, 2 },
    { "detected memory leaks",   "Memory Leak",
      "Memory was allocated but never freed. Add free() for every malloc/calloc/realloc call. Track all allocations and ensure they are released.",
      ERR_MEMORY_LEAK, 1 },
    { "SEGV on unknown address", "NULL Pointer Dereference",
      "You dereferenced a NULL or invalid pointer. Add a NULL check before accessing the pointer, and verify all pointer assignments.",
      ERR_NULL_DEREF, 2 },
    { "store to null pointer",   "NULL Pointer Dereference",
      "You wrote to a NULL pointer. Check that the pointer is valid before writing to it. Initialize pointers to NULL and verify after allocation.",
      ERR_NULL_DEREF, 2 },
    { "load of null pointer",    "NULL Pointer Dereference",
      "You read from a NULL pointer. Check that the pointer is valid before reading from it. Initialize pointers to NULL and verify after allocation.",
      ERR_NULL_DEREF, 2 },
    { "signed integer overflow", "Integer Overflow",
      "An arithmetic operation exceeded the range of a signed integer. Use int64_t for large values, or add overflow checks before the operation.",
      ERR_INT_OVERFLOW, 1 },
    { "division by zero",        "Division by Zero",
      "You divided by zero. Check that the divisor is non-zero before performing the division.",
      ERR_DIV_BY_ZERO, 2 },
    { "uninitialized value",     "Uninitialized Variable",
      "A variable was used before being initialized. Assign a default value at declaration or ensure all code paths initialize the variable.",
      ERR_UNINIT_VAR, 1 },
    { "null pointer passed",     "NULL Pointer Passed to Function",
      "A NULL pointer was passed to a function that doesn't accept it. Check return values from malloc and other functions before passing them.",
      ERR_NULL_DEREF, 2 },
    { "stack-overflow",          "Stack Overflow",
      "The call stack exceeded its limit. This is usually caused by infinite recursion. Check your recursive function's base case.",
      ERR_STACK_OVERFLOW, 2 },
    { "double-free",             "Double Free",
      "You called free() twice on the same pointer. Set the pointer to NULL after free() and check for NULL before freeing.",
      ERR_USE_AFTER_FREE, 2 },
    { "stack-buffer-underflow",  "Buffer Underflow",
      "You accessed memory before the start of a stack buffer. Check array indices for negative values or off-by-one errors.",
      ERR_BUFFER_OVERFLOW, 2 },
    { "heap-buffer-underflow",   "Buffer Underflow",
      "You accessed memory before the start of a heap buffer. Check array indices for negative values or off-by-one errors.",
      ERR_BUFFER_OVERFLOW, 2 },
    { "shift exponent",          "Invalid Shift",
      "A shift operation used an invalid exponent (negative or >= type width). Ensure shift amounts are within valid range.",
      ERR_INT_OVERFLOW, 1 },
    { "free-nonheap",            "Invalid Free",
      "You called free() on memory that was not allocated by malloc/calloc/realloc. Only free dynamically allocated memory.",
      ERR_UNKNOWN, 2 },
};

static const int num_patterns = sizeof(error_patterns) / sizeof(error_patterns[0]);

/*
 * classify_error - Determine error type from error line
 *
 * WHY: Sanitizer output contains specific keywords that identify
 *      the type of error (e.g., "heap-buffer-overflow").
 */
ErrorType classify_error(const char *error_line)
{
    int i;

    if (!error_line)
        return ERR_UNKNOWN;

    for (i = 0; i < num_patterns; i++) {
        if (string_contains(error_line, error_patterns[i].pattern))
            return error_patterns[i].type;
    }

    return ERR_UNKNOWN;
}

/*
 * parse_sanitizer_errors - Extract errors from sanitizer output
 *
 * WHY: Sanitizer output is multi-line with specific format.
 *      Parse it to extract structured error information.
 */
int parse_sanitizer_errors(const char *error_output,
                           DetectedError *errors, int max_errors)
{
    char line[MAX_LINE_LEN];
    const char *pos, *line_start, *line_end;
    int count = 0;
    ErrorType type;
    unsigned int seen_types = 0;

    if (!error_output || !errors || max_errors <= 0)
        return 0;

    pos = error_output;
    while (*pos && count < max_errors) {
        line_start = pos;
        line_end = strchr(pos, '\n');
        if (!line_end)
            line_end = pos + strlen(pos);

        size_t len = line_end - line_start;
        if (len >= MAX_LINE_LEN)
            len = MAX_LINE_LEN - 1;
        memcpy(line, line_start, len);
        line[len] = '\0';
        trim_whitespace(line);

        type = classify_error(line);
        if (type != ERR_UNKNOWN && type != ERR_NONE) {
            unsigned int type_bit = (1u << type);
            if (seen_types & type_bit) {
                pos = *line_end ? line_end + 1 : line_end;
                continue;
            }
            seen_types |= type_bit;

            errors[count].type = type;
            errors[count].severity = 0;
            errors[count].source_line = 0;
            errors[count].has_source = false;
            errors[count].source_file[0] = '\0';

            /* Find the pattern entry for title and fix */
            int i;
            for (i = 0; i < num_patterns; i++) {
                if (error_patterns[i].type == type) {
                    snprintf(errors[count].title, sizeof(errors[count].title),
                             "%s", error_patterns[i].title);
                    snprintf(errors[count].fix_suggestion,
                             sizeof(errors[count].fix_suggestion),
                             "%s", error_patterns[i].fix);
                    errors[count].severity = error_patterns[i].severity;
                    break;
                }
            }

            /* Try to extract file:line from nearby lines */
            const char *file_ref = strstr(line, ".c:");
            if (file_ref) {
                const char *path_start = file_ref;
                while (path_start > line && *(path_start - 1) != ' ' &&
                       *(path_start - 1) != '\t')
                    path_start--;
                size_t file_len = file_ref - path_start + 2;
                if (file_len < MAX_PATH_LEN - 1) {
                    memcpy(errors[count].source_file, path_start, file_len);
                    errors[count].source_file[file_len] = '\0';
                    errors[count].has_source = true;
                }
                errors[count].source_line = atoi(file_ref + 3);
            }

            count++;
        }

        pos = *line_end ? line_end + 1 : line_end;
    }

    return count;
}

/*
 * parse_signal_errors - Detect errors from signal termination
 *
 * WHY: When a program crashes with a signal (SIGSEGV, SIGABRT, etc.),
 *      we can classify the error based on the signal received.
 */
int parse_signal_errors(const char *error_output, int exit_code,
                        int signal, DetectedError *errors, int max_errors)
{
    (void)error_output;
    if (!errors || max_errors <= 0)
        return 0;

    if (signal == SIGSEGV || exit_code == 139) {
        errors[0].type = ERR_SEGV;
        snprintf(errors[0].title, sizeof(errors[0].title),
                 "Segmentation Fault");
        snprintf(errors[0].fix_suggestion, sizeof(errors[0].fix_suggestion),
                 "The program accessed invalid memory. Check for NULL pointer dereferences, buffer overflows, or use-after-free bugs. Compile with -fsanitize=address for detailed diagnostics.");
        errors[0].severity = 2;
        errors[0].source_line = 0;
        errors[0].has_source = false;
        return 1;
    }

    if (signal == SIGABRT || exit_code == 134) {
        errors[0].type = ERR_ABORT;
        snprintf(errors[0].title, sizeof(errors[0].title),
                 "Program Aborted");
        snprintf(errors[0].fix_suggestion, sizeof(errors[0].fix_suggestion),
                 "The program called abort(). Check for failed assertions, double-free errors, or heap corruption. Compile with -fsanitize=address for detailed diagnostics.");
        errors[0].severity = 2;
        errors[0].source_line = 0;
        errors[0].has_source = false;
        return 1;
    }

    return 0;
}

/*
 * generate_fix_suggestion - Create fix suggestion for detected error
 *
 * WHY: The tool should be helpful by suggesting how to fix
 *      each type of error, not just report it.
 */
void generate_fix_suggestion(DetectedError *error)
{
    int i;

    if (!error || error->fix_suggestion[0] != '\0')
        return;

    for (i = 0; i < num_patterns; i++) {
        if (error_patterns[i].type == error->type) {
            snprintf(error->fix_suggestion, sizeof(error->fix_suggestion),
                     "%s", error_patterns[i].fix);
            return;
        }
    }

    snprintf(error->fix_suggestion, sizeof(error->fix_suggestion),
             "Unknown error type. Review the code around the reported location.");
}

/* Get human-readable name for error type */
const char *get_error_name(ErrorType type)
{
    switch (type) {
    case ERR_NULL_DEREF:      return "NULL Pointer Dereference";
    case ERR_BUFFER_OVERFLOW: return "Buffer Overflow";
    case ERR_USE_AFTER_FREE:  return "Use After Free";
    case ERR_MEMORY_LEAK:     return "Memory Leak";
    case ERR_INT_OVERFLOW:    return "Integer Overflow";
    case ERR_DIV_BY_ZERO:     return "Division by Zero";
    case ERR_UNINIT_VAR:      return "Uninitialized Variable";
    case ERR_STACK_OVERFLOW:  return "Stack Overflow";
    case ERR_SEGV:            return "Segmentation Fault";
    case ERR_ABORT:           return "Program Aborted";
    case ERR_UNKNOWN:         return "Unknown Error";
    case ERR_NONE:            return "No Error";
    }
    return "Unknown Error";
}

/*
 * get_source_line - Read a specific line from source file
 *
 * WHY: When reporting errors, showing the offending line of code
 *      helps the user understand and fix the problem.
 */
int get_source_line(const char *source_file, int line_number,
                    char *buffer, size_t buffer_size)
{
    FILE *fp;
    char line[MAX_SOURCE_LINE];
    int current = 0;

    fp = fopen(source_file, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp)) {
        current++;
        if (current == line_number) {
            trim_whitespace(line);
            snprintf(buffer, buffer_size, "%s", line);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return -1;
}

/*
 * init_colors - Initialize color codes based on whether colors are enabled
 *
 * WHY: Terminal colors help distinguish error types visually.
 *      Auto-detect TTY to avoid sending escape codes to pipes/files.
 */
void init_colors(ColorCodes *colors, bool use_colors)
{
    if (!colors)
        return;

    if (use_colors) {
        colors->red = "\033[31m";
        colors->green = "\033[32m";
        colors->yellow = "\033[33m";
        colors->blue = "\033[34m";
        colors->magenta = "\033[35m";
        colors->cyan = "\033[36m";
        colors->bold = "\033[1m";
        colors->reset = "\033[0m";
    } else {
        colors->red = "";
        colors->green = "";
        colors->yellow = "";
        colors->blue = "";
        colors->magenta = "";
        colors->cyan = "";
        colors->bold = "";
        colors->reset = "";
    }
}

/*
 * print_colored - Print formatted message with color
 *
 * WHY: Color-coded output makes errors stand out and improves
 *      readability of test results in the terminal.
 */
void print_colored(const ColorCodes *colors, const char *color,
                   const char *fmt, ...)
{
    va_list args;

    if (!colors || !fmt)
        return;

    if (color)
        printf("%s", color);

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (color && colors->reset)
        printf("%s", colors->reset);
}

/*
 * print_banner - Print banner showing file being tested
 *
 * WHY: Visual header helps user identify which file is being
 *      tested, especially when running multiple tests.
 */
void print_banner(const ColorCodes *colors, const char *filename)
{
    if (!colors || !filename)
        return;

    printf("========================================\n");
    print_colored(colors, colors->bold, "  c_tester - C Error Detection Tool\n");
    printf("========================================\n");
    print_colored(colors, colors->cyan, "Testing: ");
    printf("%s\n", filename);
    printf("----------------------------------------\n");
}

/*
 * print_summary - Print summary of test results
 *
 * WHY: Summarize findings for the user with clear pass/fail
 *      indication and actionable fix suggestions.
 */
void print_summary(const ColorCodes *colors, const TestResult *result,
                   int error_count, const DetectedError *errors,
                   const char *source_file)
{
    char source_line[MAX_SOURCE_LINE];
    int i;

    if (!colors || !result)
        return;

    printf("\n");

    if (error_count > 0) {
        for (i = 0; i < error_count; i++) {
            print_colored(colors, colors->red, "[ERROR] ");
            print_colored(colors, colors->bold, "%s", errors[i].title);
            if (errors[i].has_source && errors[i].source_line > 0)
                printf(" (%s:%d)", errors[i].source_file,
                       errors[i].source_line);
            printf("\n");

            print_colored(colors, colors->yellow, "  Fix: ");
            printf("%s\n", errors[i].fix_suggestion);

            if (errors[i].has_source && errors[i].source_line > 0 &&
                get_source_line(source_file, errors[i].source_line,
                                source_line, sizeof(source_line)) == 0) {
                print_colored(colors, colors->cyan, "  -> %d | ",
                              errors[i].source_line);
                printf("%s\n", source_line);
            }
            printf("\n");
        }
        print_colored(colors, colors->red, "[ERROR] ");
        printf("%d error(s) detected\n", error_count);
    } else if (result->compilation_success) {
        print_colored(colors, colors->green, "[OK] ");
        print_colored(colors, colors->bold,
                      "No errors detected - clean run in %ldms\n",
                      result->execution_time_ms);
    } else {
        print_colored(colors, colors->red, "[COMPILE ERROR]\n");
        if (result->compiler_output[0])
            printf("%s\n", result->compiler_output);
    }
}

/*
 * main - CLI entry point for c_tester
 *
 * WHY: Parse command-line arguments, orchestrate the compilation,
 *      execution, and error reporting workflow.
 */
int main(int argc, char *argv[])
{
    ColorCodes colors;
    TestResult result;
    DetectedError errors[32];
    char binary_path[MAX_PATH_LEN];
    const char *source_file = NULL;
    bool keep_binary = false;
    bool use_color = true;
    int timeout_sec = DEFAULT_TIMEOUT_SEC;
    int error_count = 0;
    int i;
    use_color = isatty(STDOUT_FILENO);
    init_colors(&colors, use_color);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--keep") == 0) {
            keep_binary = true;
        } else if (strncmp(argv[i], "--timeout=", 10) == 0) {
            timeout_sec = atoi(argv[i] + 10);
            if (timeout_sec <= 0)
                timeout_sec = DEFAULT_TIMEOUT_SEC;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            use_color = false;
            init_colors(&colors, use_color);
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        }
    }

    if (!source_file) {
        print_colored(&colors, colors.bold, "Usage: ");
        printf("%s [options] <source.c>\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --keep         Keep compiled binary after run\n");
        printf("  --timeout=N    Set execution timeout in seconds (default: %d)\n",
               DEFAULT_TIMEOUT_SEC);
        printf("  --no-color     Disable colored output\n");
        return EXIT_USAGE_ERROR;
    }

    if (!file_exists(source_file)) {
        print_colored(&colors, colors.red, "Error: ");
        printf("file not found: %s\n", source_file);
        return EXIT_FILE_NOT_FOUND;
    }

    if (!is_c_file(source_file)) {
        print_colored(&colors, colors.red, "Error: ");
        printf("not a C source file: %s\n", source_file);
        return EXIT_USAGE_ERROR;
    }

    print_banner(&colors, source_file);

    generate_temp_path("c_tester", binary_path, sizeof(binary_path));

    memset(&result, 0, sizeof(result));

    if (compile_with_sanitizers(source_file, binary_path,
                                 result.compiler_output,
                                 sizeof(result.compiler_output)) == 0) {
        result.compilation_success = true;
    } else if (compile_fallback(source_file, binary_path,
                                 result.compiler_output,
                                 sizeof(result.compiler_output)) == 0) {
        result.compilation_success = true;
    }

    if (!result.compilation_success) {
        print_colored(&colors, colors.red, "[COMPILE ERROR]\n");
        if (result.compiler_output[0])
            printf("%s\n", result.compiler_output);
        return EXIT_COMPILE_FAIL;
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    setenv("ASAN_OPTIONS", "detect_leaks=1", 1);

    result.exit_code = run_binary(binary_path, NULL,
                                   result.runtime_output,
                                   sizeof(result.runtime_output),
                                   result.sanitizer_output,
                                   sizeof(result.sanitizer_output),
                                   timeout_sec, NULL);

    if (result.exit_code == -1) {
        snprintf(result.sanitizer_output, sizeof(result.sanitizer_output),
                 "Execution timeout after %d seconds", timeout_sec);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    get_execution_time(&start_time, &end_time, &result.execution_time_ms);

    /* Parse sanitizer errors */
    int sanitizer_count = parse_sanitizer_errors(result.sanitizer_output,
                                                  errors + error_count,
                                                  32 - error_count);
    error_count += sanitizer_count;

    /* Only report compiler warnings when no sanitizer errors found */
    if (error_count == 0 &&
        string_contains(result.compiler_output, "warning:")) {
        error_count = 1;
        if (string_contains(result.compiler_output, "uninitialized"))
            errors[0].type = ERR_UNINIT_VAR;
        else
            errors[0].type = ERR_UNKNOWN;
        snprintf(errors[0].title, sizeof(errors[0].title),
                 "%s", get_error_name(errors[0].type));
        char warning_excerpt[512];
        strncpy(warning_excerpt, result.compiler_output, sizeof(warning_excerpt) - 1);
        warning_excerpt[sizeof(warning_excerpt) - 1] = '\0';
        snprintf(errors[0].fix_suggestion, sizeof(errors[0].fix_suggestion),
                 "Compiler detected: %s", warning_excerpt);
        errors[0].severity = 1;
        errors[0].has_source = false;
    }

    if (error_count == 0 && result.exit_code == -1) {
        errors[0].type = ERR_UNKNOWN;
        snprintf(errors[0].title, sizeof(errors[0].title),
                 "Execution Timeout");
        snprintf(errors[0].fix_suggestion, sizeof(errors[0].fix_suggestion),
                 "The program exceeded the %d second timeout. Check for infinite loops or blocking operations.",
                 timeout_sec);
        errors[0].severity = 1;
        error_count = 1;
    }

    if (error_count == 0 && result.exit_code != 0 &&
        result.exit_code != EXIT_CLEAN) {
        error_count = parse_signal_errors(result.sanitizer_output,
                                           result.exit_code,
                                           result.exit_code > 128 ?
                                               result.exit_code - 128 : 0,
                                           errors, 32);
    }

    /* Generate fix suggestions and get source context */
    for (i = 0; i < error_count; i++)
        generate_fix_suggestion(&errors[i]);

    /* Print summary */
    print_summary(&colors, &result, error_count, errors, source_file);

    /* Cleanup */
    if (!keep_binary)
        cleanup_binary(binary_path);

    return error_count > 0 ? EXIT_ERRORS_FOUND : EXIT_CLEAN;
}
