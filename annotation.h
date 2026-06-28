/*
 * annotation.h - Function annotation system for prism
 *
 * Describes ~50 libc function contracts: param roles (buffer, size, ptr, fd,
 * format-str), alloc/free and open/close pairings, and return-value semantics.
 * Built-in DB compiled in; optional JSON overrides from a file.
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include "prism.h"
#include <stdbool.h>

/* Parameter and function roles */

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

/* Annotation for one function */
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

/* Annotation Database (opaque) */
typedef struct AnnotationDB AnnotationDB;

/* Load annotations: built-in if path is NULL, + JSON overrides if given */
AnnotationDB *ann_load(const char *path);

/* Look up a function's annotation; NULL if not found */
const Annotation *ann_lookup(AnnotationDB *db, const char *func_name);

/*
 * ann_free - Free annotation database
 */
void ann_free(AnnotationDB *db);

/* True if a and b are an alloc/free or open/close pair (either direction) */
bool ann_is_paired(const Annotation *a, const Annotation *b);

#endif /* ANNOTATION_H */
