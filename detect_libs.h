/*
 * detect_libs.h - Auto-detect library deps from #include headers
 *
 * Scans sources for #include <...> directives, looks them up in a
 * 50-entry table (GMP, pthread, X11, SDL2, OpenSSL, curl, GTK, Qt, ...),
 * falls back to pkg-config(1). Results cached after first call.
 */

#ifndef DETECT_LIBS_H
#define DETECT_LIBS_H

#include <stdbool.h>
#include <stddef.h>

/* Scan sources and build a linker flags string */
int detect_libraries(const char **sources, int source_count,
                     char *output, size_t output_size);

/*
 * get_detected_libs - Returns the last detected library flags (cached)
 */
const char *get_detected_libs(void);

/* Generate install-command hints for missing packages */
int get_suggested_packages(const char **sources, int source_count,
                           char *output, size_t output_size);

#endif /* DETECT_LIBS_H */
