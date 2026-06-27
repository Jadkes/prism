/*
 * detect_libs.h - Auto-detect library dependencies from C/C++ source headers
 *
 * Purpose: Scan source files for #include <...> directives, look them up in
 *          a known-header table (covering GMP, pthread, X11, SDL2, OpenSSL,
 *          curl, ncurses, readline, GTK, Qt, and 40+ more), and fall back to
 *          pkg-config(1) for anything not in the table.
 *
 * Design: The table is ordered by header name. Each entry stores the pkg-config
 *         package name (or NULL) and raw linker flags.  Entries with a
 *         pkg-config name are verified on first use: if pkg-config succeeds,
 *         its --cflags --libs output is used instead of the raw flags.
 *
 *         Results are cached after the first call so repeated lookups in
 *         different compile functions are free.
 *
 * Thread-safety: Single-threaded tool, no concurrency concerns.
 */

#ifndef DETECT_LIBS_H
#define DETECT_LIBS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * detect_libraries - Scan sources and build library flags string
 *
 * @param sources     - Array of source file paths
 * @param source_count - Number of source files
 * @param output      - Buffer to receive flags string (e.g. "-lgmp -lm")
 * @param output_size - Size of output buffer
 * @return 0 on success, -1 on failure
 */
int detect_libraries(const char **sources, int source_count,
                     char *output, size_t output_size);

/*
 * get_detected_libs - Returns the last detected library flags (cached)
 *
 * @return Pointer to static buffer containing flags string, or "" if none
 */
const char *get_detected_libs(void);

/*
 * get_suggested_packages - Format install-command hints for missing deps
 *
 * WHY: When compilation fails with undefined references, show the user
 *      which packages they need to install.
 *
 * @param sources     - Array of source file paths
 * @param source_count - Number of source files
 * @param output      - Buffer for human-readable hint string
 * @param output_size - Size of output buffer
 * @return Number of missing packages detected
 */
int get_suggested_packages(const char **sources, int source_count,
                           char *output, size_t output_size);

#endif /* DETECT_LIBS_H */
