/*
 * annotation.h - Function annotation system for c_tester
 *
 * Purpose: Provides function contract information for static analysis.
 *          Each annotation describes a function's parameters (buffer, size,
 *          pointer, fd, format string roles), pairing relationships
 *          (alloc↔free, open↔close), and return value semantics.
 *
 * Design: Built-in annotations for ~50 common libc functions compiled
 *         in. Optional JSON file loading for project-specific functions.
 *         Phase 2 foundation; consumed by check modules in Phase 3.
 *
 * Thread-safety: Single-threaded. DB is loaded once and read-only after.
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include "c_tester.h"
#include <stdbool.h>

/* ---- Parameter and function role enums ---- */

typedef enum {
    ROLE_BUFFER,        /* Destination buffer (written to) */
    ROLE_SOURCE,        /* Source data (read from) */
    ROLE_BUFFER_SIZE,   /* Sizeof a corresponding buffer */
    ROLE_SIZE,          /* Byte count (allocation size, read count) */
    ROLE_PTR,           /* Opaque pointer (freed, closed, etc.) */
    ROLE_FMT_STR,       /* Printf-style format string */
    ROLE_FD,            /* File descriptor */
    ROLE_FLAGS,         /* Flags / mode / options */
    ROLE_UNKNOWN
} ParamRole;

/* A parameter with its name and role */
typedef struct {
    const char *name;
    ParamRole role;
} ParamInfo;

/* ---- Annotation for a single function ---- */
typedef struct {
    const char *func_name;

    /* Parameter roles */
    ParamInfo params[8];
    int param_count;

    /* Pair tracking: allocators ↔ deallocators, open ↔ close */
    const char *paired_alloc;   /* e.g. "free" is paired with "malloc" */
    const char *paired_free;    /* e.g. "malloc" is paired with "free"  */
    const char *paired_open;    /* e.g. "fopen" is paired with "fclose" */
    const char *paired_close;   /* e.g. "fclose" is paired with "fopen" */

    /* Return value semantics */
    bool returns_alloc;  /* Returns a newly-allocated pointer */
    bool returns_fd;     /* Returns a file descriptor */
    bool returns_size;   /* Returns number of bytes (e.g., read) */
    bool returns_bool;   /* Returns 0 on success, non-zero on error */
    bool can_return_null;/* Returns NULL on failure (alloc, getenv, fopen) */
} Annotation;

/* ---- Annotation Database ---- */
typedef struct AnnotationDB AnnotationDB;

/*
 * ann_load - Load annotations
 *
 * If path is NULL, loads only the built-in annotations (~50 libc funcs).
 * If path is non-NULL, loads built-in + JSON overrides from file.
 *
 * @param path - Optional path to JSON annotations file (NULL for built-in)
 * @return AnnotationDB, or NULL on failure. Must be ann_free()'d.
 */
AnnotationDB *ann_load(const char *path);

/*
 * ann_lookup - Get annotation for a function
 *
 * @param db - Annotation database
 * @param func_name - Exact function name
 * @return Pointer to annotation, or NULL if not found
 */
const Annotation *ann_lookup(AnnotationDB *db, const char *func_name);

/*
 * ann_free - Free annotation database
 */
void ann_free(AnnotationDB *db);

/*
 * ann_is_paired - Check if two functions are a resource pair
 *
 * Returns true if a and b form an alloc/free or open/close pair
 * (in either direction).
 */
bool ann_is_paired(const Annotation *a, const Annotation *b);

#endif /* ANNOTATION_H */
