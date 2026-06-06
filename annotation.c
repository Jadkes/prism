/*
 * annotation.c - Function annotation system implementation
 *
 * Purpose: Contains the built-in annotation database (~50 libc functions)
 *          and the lookup API consumed by check modules in Phase 3.
 *          The annotation array is sorted lexicographically and searched
 *          via binary search for O(log n) lookup.
 */

#include "annotation.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---- Built-in annotation array ---- */
/* Sorted alphabetically for binary search. */
/* Initialized with a function to avoid macro comma issues with {} initializers. */

static void init_builtin(Annotation *ann, int *count_out)
{
    int i = 0;

    /* aligned_alloc */
    ann[i].func_name = "aligned_alloc";
    ann[i].params[0].name = "alignment"; ann[i].params[0].role = ROLE_SIZE;
    ann[i].params[1].name = "size";      ann[i].params[1].role = ROLE_SIZE;
    ann[i].param_count = 2;
    ann[i].paired_free = "free";
    ann[i].returns_alloc = true;
    ann[i].can_return_null = true;
    i++;

    /* calloc */
    ann[i].func_name = "calloc";
    ann[i].params[0].name = "nmemb"; ann[i].params[0].role = ROLE_SIZE;
    ann[i].params[1].name = "size";  ann[i].params[1].role = ROLE_SIZE;
    ann[i].param_count = 2;
    ann[i].paired_free = "free";
    ann[i].returns_alloc = true;
    ann[i].can_return_null = true;
    i++;

    /* malloc */
    ann[i].func_name = "malloc";
    ann[i].params[0].name = "size"; ann[i].params[0].role = ROLE_SIZE;
    ann[i].param_count = 1;
    ann[i].paired_free = "free";
    ann[i].returns_alloc = true;
    ann[i].can_return_null = true;
    i++;

    /* realloc */
    ann[i].func_name = "realloc";
    ann[i].params[0].name = "ptr";  ann[i].params[0].role = ROLE_PTR;
    ann[i].params[1].name = "size"; ann[i].params[1].role = ROLE_SIZE;
    ann[i].param_count = 2;
    ann[i].paired_free = "free";
    ann[i].returns_alloc = true;
    ann[i].can_return_null = true;
    i++;

    /* strdup */
    ann[i].func_name = "strdup";
    ann[i].params[0].name = "s"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].param_count = 1;
    ann[i].paired_free = "free";
    ann[i].returns_alloc = true;
    ann[i].can_return_null = true;
    i++;

    /* free */
    ann[i].func_name = "free";
    ann[i].params[0].name = "ptr"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    ann[i].paired_alloc = "malloc";
    i++;

    /* memcpy */
    ann[i].func_name = "memcpy";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "n";    ann[i].params[2].role = ROLE_SIZE;
    ann[i].param_count = 3;
    i++;

    /* memmove */
    ann[i].func_name = "memmove";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "n";    ann[i].params[2].role = ROLE_SIZE;
    ann[i].param_count = 3;
    i++;

    /* memset */
    ann[i].func_name = "memset";
    ann[i].params[0].name = "s"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "c"; ann[i].params[1].role = ROLE_FLAGS;
    ann[i].params[2].name = "n"; ann[i].params[2].role = ROLE_SIZE;
    ann[i].param_count = 3;
    i++;

    /* strcpy */
    ann[i].func_name = "strcpy";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].param_count = 2;
    i++;

    /* strncpy */
    ann[i].func_name = "strncpy";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "n";    ann[i].params[2].role = ROLE_BUFFER_SIZE;
    ann[i].param_count = 3;
    i++;

    /* strcat */
    ann[i].func_name = "strcat";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].param_count = 2;
    i++;

    /* strncat */
    ann[i].func_name = "strncat";
    ann[i].params[0].name = "dest"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "src";  ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "n";    ann[i].params[2].role = ROLE_BUFFER_SIZE;
    ann[i].param_count = 3;
    i++;

    /* printf */
    ann[i].func_name = "printf";
    ann[i].params[0].name = "fmt"; ann[i].params[0].role = ROLE_FMT_STR;
    ann[i].param_count = 1;
    i++;

    /* fprintf */
    ann[i].func_name = "fprintf";
    ann[i].params[0].name = "stream"; ann[i].params[0].role = ROLE_PTR;
    ann[i].params[1].name = "fmt";    ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* dprintf */
    ann[i].func_name = "dprintf";
    ann[i].params[0].name = "fd";  ann[i].params[0].role = ROLE_FD;
    ann[i].params[1].name = "fmt"; ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* sprintf */
    ann[i].func_name = "sprintf";
    ann[i].params[0].name = "buf"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "fmt"; ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* snprintf */
    ann[i].func_name = "snprintf";
    ann[i].params[0].name = "buf";  ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "size"; ann[i].params[1].role = ROLE_BUFFER_SIZE;
    ann[i].params[2].name = "fmt";  ann[i].params[2].role = ROLE_FMT_STR;
    ann[i].param_count = 3;
    i++;

    /* asprintf */
    ann[i].func_name = "asprintf";
    ann[i].params[0].name = "strp"; ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "fmt";  ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    ann[i].paired_free = "free";
    i++;

    /* scanf */
    ann[i].func_name = "scanf";
    ann[i].params[0].name = "fmt"; ann[i].params[0].role = ROLE_FMT_STR;
    ann[i].param_count = 1;
    i++;

    /* fscanf */
    ann[i].func_name = "fscanf";
    ann[i].params[0].name = "stream"; ann[i].params[0].role = ROLE_PTR;
    ann[i].params[1].name = "fmt";    ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* sscanf */
    ann[i].func_name = "sscanf";
    ann[i].params[0].name = "str"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "fmt"; ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* syslog */
    ann[i].func_name = "syslog";
    ann[i].params[0].name = "priority"; ann[i].params[0].role = ROLE_FLAGS;
    ann[i].params[1].name = "fmt";      ann[i].params[1].role = ROLE_FMT_STR;
    ann[i].param_count = 2;
    i++;

    /* fopen */
    ann[i].func_name = "fopen";
    ann[i].params[0].name = "path"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "mode"; ann[i].params[1].role = ROLE_FLAGS;
    ann[i].param_count = 2;
    ann[i].paired_close = "fclose";
    ann[i].can_return_null = true;
    i++;

    /* fclose */
    ann[i].func_name = "fclose";
    ann[i].params[0].name = "stream"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    ann[i].paired_open = "fopen";
    ann[i].returns_bool = true;
    i++;

    /* fread */
    ann[i].func_name = "fread";
    ann[i].params[0].name = "ptr";    ann[i].params[0].role = ROLE_BUFFER;
    ann[i].params[1].name = "size";   ann[i].params[1].role = ROLE_SIZE;
    ann[i].params[2].name = "nmemb";  ann[i].params[2].role = ROLE_SIZE;
    ann[i].params[3].name = "stream"; ann[i].params[3].role = ROLE_PTR;
    ann[i].param_count = 4;
    i++;

    /* fwrite */
    ann[i].func_name = "fwrite";
    ann[i].params[0].name = "ptr";    ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "size";   ann[i].params[1].role = ROLE_SIZE;
    ann[i].params[2].name = "nmemb";  ann[i].params[2].role = ROLE_SIZE;
    ann[i].params[3].name = "stream"; ann[i].params[3].role = ROLE_PTR;
    ann[i].param_count = 4;
    ann[i].returns_size = true;
    i++;

    /* ferror */
    ann[i].func_name = "ferror";
    ann[i].params[0].name = "stream"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    i++;

    /* open */
    ann[i].func_name = "open";
    ann[i].params[0].name = "path";  ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "flags"; ann[i].params[1].role = ROLE_FLAGS;
    ann[i].param_count = 2;
    ann[i].paired_close = "close";
    ann[i].returns_fd = true;
    i++;

    /* close */
    ann[i].func_name = "close";
    ann[i].params[0].name = "fd"; ann[i].params[0].role = ROLE_FD;
    ann[i].param_count = 1;
    ann[i].paired_open = "open";
    i++;

    /* read */
    ann[i].func_name = "read";
    ann[i].params[0].name = "fd";    ann[i].params[0].role = ROLE_FD;
    ann[i].params[1].name = "buf";   ann[i].params[1].role = ROLE_BUFFER;
    ann[i].params[2].name = "count"; ann[i].params[2].role = ROLE_BUFFER_SIZE;
    ann[i].param_count = 3;
    ann[i].returns_size = true;
    i++;

    /* write */
    ann[i].func_name = "write";
    ann[i].params[0].name = "fd";    ann[i].params[0].role = ROLE_FD;
    ann[i].params[1].name = "buf";   ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "count"; ann[i].params[2].role = ROLE_BUFFER_SIZE;
    ann[i].param_count = 3;
    ann[i].returns_size = true;
    i++;

    /* socket */
    ann[i].func_name = "socket";
    ann[i].params[0].name = "domain";   ann[i].params[0].role = ROLE_FLAGS;
    ann[i].params[1].name = "type";     ann[i].params[1].role = ROLE_FLAGS;
    ann[i].params[2].name = "protocol"; ann[i].params[2].role = ROLE_FLAGS;
    ann[i].param_count = 3;
    ann[i].paired_close = "close";
    ann[i].returns_fd = true;
    i++;

    /* accept */
    ann[i].func_name = "accept";
    ann[i].params[0].name = "sockfd";  ann[i].params[0].role = ROLE_FD;
    ann[i].params[1].name = "addr";    ann[i].params[1].role = ROLE_BUFFER;
    ann[i].params[2].name = "addrlen"; ann[i].params[2].role = ROLE_BUFFER_SIZE;
    ann[i].param_count = 3;
    ann[i].paired_close = "close";
    ann[i].returns_fd = true;
    ann[i].can_return_null = true;
    i++;

    /* pthread_mutex_lock */
    ann[i].func_name = "pthread_mutex_lock";
    ann[i].params[0].name = "mutex"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    ann[i].paired_close = "pthread_mutex_unlock";
    i++;

    /* pthread_mutex_unlock */
    ann[i].func_name = "pthread_mutex_unlock";
    ann[i].params[0].name = "mutex"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    ann[i].paired_open = "pthread_mutex_lock";
    i++;

    /* pthread_mutex_destroy */
    ann[i].func_name = "pthread_mutex_destroy";
    ann[i].params[0].name = "mutex"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    i++;

    /* getenv */
    ann[i].func_name = "getenv";
    ann[i].params[0].name = "name"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].param_count = 1;
    ann[i].can_return_null = true;
    i++;

    /* setjmp */
    ann[i].func_name = "setjmp";
    ann[i].params[0].name = "env"; ann[i].params[0].role = ROLE_PTR;
    ann[i].param_count = 1;
    i++;

    /* longjmp */
    ann[i].func_name = "longjmp";
    ann[i].params[0].name = "env"; ann[i].params[0].role = ROLE_PTR;
    ann[i].params[1].name = "val"; ann[i].params[1].role = ROLE_FLAGS;
    ann[i].param_count = 2;
    i++;

    /* strlen */
    ann[i].func_name = "strlen";
    ann[i].params[0].name = "s"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].param_count = 1;
    i++;

    /* strcmp */
    ann[i].func_name = "strcmp";
    ann[i].params[0].name = "s1"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "s2"; ann[i].params[1].role = ROLE_SOURCE;
    ann[i].param_count = 2;
    i++;

    /* strncmp */
    ann[i].func_name = "strncmp";
    ann[i].params[0].name = "s1"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "s2"; ann[i].params[1].role = ROLE_SOURCE;
    ann[i].params[2].name = "n";  ann[i].params[2].role = ROLE_SIZE;
    ann[i].param_count = 3;
    i++;

    /* strstr */
    ann[i].func_name = "strstr";
    ann[i].params[0].name = "haystack"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "needle";   ann[i].params[1].role = ROLE_SOURCE;
    ann[i].param_count = 2;
    i++;

    /* strchr */
    ann[i].func_name = "strchr";
    ann[i].params[0].name = "s"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].params[1].name = "c"; ann[i].params[1].role = ROLE_FLAGS;
    ann[i].param_count = 2;
    i++;

    /* atoi */
    ann[i].func_name = "atoi";
    ann[i].params[0].name = "nptr"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].param_count = 1;
    i++;

    /* atol */
    ann[i].func_name = "atol";
    ann[i].params[0].name = "nptr"; ann[i].params[0].role = ROLE_SOURCE;
    ann[i].param_count = 1;
    i++;

    /* assert */
    ann[i].func_name = "assert";
    ann[i].params[0].name = "expr"; ann[i].params[0].role = ROLE_FLAGS;
    ann[i].param_count = 1;
    i++;

    *count_out = i;
}

#define BUILTIN_MAX 64

/* ---- Annotation Database ---- */

struct AnnotationDB {
    Annotation builtin[BUILTIN_MAX];
    int builtin_count;
    Annotation *user_annots;
    int user_count;
};

/* ---- Binary search ---- */
static const Annotation *find_annotation(const Annotation *array,
                                          int count, const char *name)
{
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(name, array[mid].func_name);
        if (cmp == 0) return &array[mid];
        if (cmp < 0)  hi = mid - 1;
        else           lo = mid + 1;
    }
    return NULL;
}

static int annot_cmp(const void *a, const void *b)
{
    const Annotation *pa = (const Annotation *)a;
    const Annotation *pb = (const Annotation *)b;
    return strcmp(pa->func_name, pb->func_name);
}

/* ---- Public API ---- */

AnnotationDB *ann_load(const char *path)
{
    AnnotationDB *db = calloc(1, sizeof(AnnotationDB));
    if (!db) return NULL;

    init_builtin(db->builtin, &db->builtin_count);
    qsort(db->builtin, db->builtin_count, sizeof(Annotation), annot_cmp);

    /* Load user annotations from JSON if path provided */
    if (path) {
        FILE *fp = fopen(path, "r");
        if (!fp) {
            fprintf(stderr, "Warning: Could not open annotations: %s\n",
                    path);
            return db;
        }

        char line[4096];
        int cap = 0;
        while (fgets(line, sizeof(line), fp)) {
            const char *p = line;
            while (*p && *p != '"') p++;
            if (!*p) continue;
            p++;
            const char *name_start = p;
            while (*p && *p != '"') p++;
            if (!*p) continue;
            int name_len = (int)(p - name_start);
            if (name_len <= 0 || name_len > 127) continue;

            if (db->user_count >= cap) {
                int newcap = cap == 0 ? 16 : cap * 2;
                Annotation *tmp = realloc(
                    db->user_annots,
                    (size_t)newcap * sizeof(Annotation));
                if (!tmp) break;
                db->user_annots = tmp;
                cap = newcap;
            }

            Annotation *a = &db->user_annots[db->user_count++];
            memset(a, 0, sizeof(Annotation));
            snprintf((char*)a->func_name, 128, "%.*s", name_len,
                     name_start);
        }
        fclose(fp);

        if (db->user_count > 1)
            qsort(db->user_annots, db->user_count,
                  sizeof(Annotation), annot_cmp);
    }

    return db;
}

const Annotation *ann_lookup(AnnotationDB *db, const char *func_name)
{
    if (!db || !func_name) return NULL;

    if (db->user_count > 0) {
        const Annotation *a = find_annotation(
            db->user_annots, db->user_count, func_name);
        if (a) return a;
    }

    return find_annotation(db->builtin, db->builtin_count, func_name);
}

void ann_free(AnnotationDB *db)
{
    if (!db) return;
    free(db->user_annots);
    free(db);
}

bool ann_is_paired(const Annotation *a, const Annotation *b)
{
    if (!a || !b) return false;

    if (a->paired_free && b->func_name &&
        strcmp(a->paired_free, b->func_name) == 0)
        return true;

    if (a->paired_alloc && b->func_name &&
        strcmp(a->paired_alloc, b->func_name) == 0)
        return true;

    if (a->paired_close && b->func_name &&
        strcmp(a->paired_close, b->func_name) == 0)
        return true;

    if (a->paired_open && b->func_name &&
        strcmp(a->paired_open, b->func_name) == 0)
        return true;

    return false;
}
