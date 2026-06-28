/*
 * check_engine.c - 8 static analysis passes over AST call-site data
 *
 * Each pass looks for a specific flavor of footgun, fed by the AST backend's
 * call-site list and the annotation system's function contracts.
 *
 *   1. null_deref_check     — ptr=NULL then ->field without a NULL check
 *   2. use_after_free_check — free(ptr) then ->field
 *   3. buffer_size_check    — strcpy/sprintf to a fixed-size buffer
 *   4. resource_leak_check  — fopen/open/socket without fclose/close
 *   5. double_free_check    — free(ptr) twice
 *   6. format_string_check  — printf(user_var) with no format string
 *   7. overflow_check       — unchecked atoi() used as array index
 *   8. null_check_order     — ptr->field before if(!ptr)
 */

#include "check_engine.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Call-site sorting by line */

static int sort_by_line(const void *a, const void *b)
{
    const CallSite *ca = (const CallSite *)a;
    const CallSite *cb = (const CallSite *)b;
    if (ca->line < cb->line) return -1;
    if (ca->line > cb->line) return  1;
    return 0;
}

/* Safe source-file copy (snprintf avoids -Wstringop-truncation) */

static void copy_source_file(char *dst, size_t dst_sz,
                              const char *src)
{
    snprintf(dst, dst_sz, "%.*s", (int)(dst_sz - 1), src);
}

/* Module 1: checking if u forgot to null check before *ptr = x */

static int check_null_deref_in_func(
    const CallSite *calls, int n_calls,
    AnnotationDB *ann,
    DetectedError *errors, int max)
{
    int found = 0;
    for (int i = 0; i < n_calls && found < max; i++) {
        const Annotation *a = ann_lookup(ann, calls[i].callee);
        if (!a || !a->can_return_null)
            continue;

        /* flagged — unchecked nullable return */
        DetectedError *de = &errors[found];
        memset(de, 0, sizeof(DetectedError));
        de->type = ERR_NULL_DEREF;
        de->severity = 2;
        de->has_source = true;
        copy_source_file(de->source_file, sizeof(de->source_file),
                         calls[i].source_file);
        de->source_line = calls[i].line;
        snprintf(de->title, sizeof(de->title),
                 "Unchecked nullable return: %s()",
                 calls[i].callee);
        snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                 "%s() can return NULL at %s:%u. Check the return "
                 "value before use.",
                 calls[i].callee, calls[i].source_file, calls[i].line);
        found++;
    }
    return found;
}

/* Module 2: did u free(ptr) and then touch it again */

static int check_uaf_in_func(
    const CallSite *calls, int n_calls,
    AnnotationDB *ann,
    DetectedError *errors, int max)
{
    int found = 0;

    /* Look for use-after-free: callee after free() */
    for (int i = 0; i < n_calls; i++) {
        if (strcmp(calls[i].callee, "free") != 0)
            continue;

        /* Any non-free use of the pointer after the free */
        for (int j = i + 1; j < n_calls; j++) {
            const Annotation *aj = ann_lookup(ann, calls[j].callee);
            if (!aj) continue;

            /* Skip free/realloc and allocators */
            if (strcmp(calls[j].callee, "free") == 0 ||
                strcmp(calls[j].callee, "realloc") == 0)
                continue;
            if (aj->returns_alloc) continue;

            /* Skip calls on the same or adjacent line */
            if (calls[j].line <= calls[i].line + 1)
                continue;

            if (found >= max) break;

            DetectedError *de = &errors[found];
            memset(de, 0, sizeof(DetectedError));
            de->type = ERR_USE_AFTER_FREE;
            de->severity = 3;
            de->has_source = true;
            copy_source_file(de->source_file, sizeof(de->source_file),
                             calls[j].source_file);
            de->source_line = calls[j].line;
            snprintf(de->title, sizeof(de->title),
                     "Potential use-after-free");
            snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                     "Call to '%s()' at %s:%u occurs after "
                     "free() at %s:%u. Verify the pointer is not "
                     "dangling.",
                     calls[j].callee, calls[j].source_file,
                     calls[j].line,
                     calls[i].source_file, calls[i].line);
            found++;
        }
    }
    return found;
}

/* Module 3: barking at unbounded strcpy/sprintf/strcat */

static int check_buffer_size(
    const CallSite *calls, int n_calls,
    DetectedError *errors, int max)
{
    int found = 0;
    static const char *unsafe[] = {"strcpy", "strcat", "sprintf", "gets", NULL};
    static const char *safe[]   = {"strncpy", "strncat", "snprintf", "fgets", NULL};

    for (int i = 0; i < n_calls && found < max; i++) {
        int unsafe_idx = -1;
        for (int u = 0; unsafe[u]; u++) {
            if (strcmp(calls[i].callee, unsafe[u]) == 0) {
                unsafe_idx = u;
                break;
            }
        }
        if (unsafe_idx < 0) continue;

        DetectedError *de = &errors[found];
        memset(de, 0, sizeof(DetectedError));
        de->type = ERR_BUFFER_OVERFLOW;
        de->severity = 3;
        de->has_source = true;
        copy_source_file(de->source_file, sizeof(de->source_file),
                         calls[i].source_file);
        de->source_line = calls[i].line;
        snprintf(de->title, sizeof(de->title),
                 "Unbounded %s() at %s:%u",
                 calls[i].callee, calls[i].source_file, calls[i].line);
        snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                 "Use %s() instead of %s(). %s does not check "
                 "destination buffer bounds.",
                 safe[unsafe_idx], unsafe[unsafe_idx], unsafe[unsafe_idx]);
        found++;
    }
    return found;
}

/* Module 4: fopen'd it, forgot to fclose */

static int check_resource_leak(
    const CallSite *calls, int n_calls,
    AnnotationDB *ann,
    DetectedError *errors, int max)
{
    int found = 0;
    enum { MAX_RES = 32 };

    struct ResourceRec {
        const char *open_func;
        const char *close_func;
        unsigned open_line;
        bool is_open;
    } resources[MAX_RES];
    int res_count = 0;

    memset(resources, 0, sizeof(resources));

    for (int i = 0; i < n_calls; i++) {
        const Annotation *a = ann_lookup(ann, calls[i].callee);
        if (!a) continue;

        if (a->paired_close) {
            /* Opening resource: fopen, open, socket, ... */
            if (res_count < MAX_RES) {
                resources[res_count].open_func = calls[i].callee;
                resources[res_count].close_func = a->paired_close;
                resources[res_count].open_line = calls[i].line;
                resources[res_count].is_open = true;
                res_count++;
            }
        } else if (a->paired_open) {
            /* Closing resource match */
            for (int r = 0; r < res_count; r++) {
                if (resources[r].is_open &&
                    strcmp(resources[r].close_func,
                           calls[i].callee) == 0) {
                    resources[r].is_open = false;
                    break;
                }
            }
        }
    }

    /* Spill: anything still open now is a leak */
    for (int r = 0; r < res_count && found < max; r++) {
        if (!resources[r].is_open) continue;

        DetectedError *de = &errors[found];
        memset(de, 0, sizeof(DetectedError));
        de->type = ERR_MEMORY_LEAK;
        de->severity = 2;
        de->has_source = true;

        /* Fish the source file from the calls array */
        for (int i = 0; i < n_calls; i++) {
            if (strcmp(calls[i].callee, resources[r].open_func) == 0 &&
                calls[i].line == resources[r].open_line) {
                copy_source_file(de->source_file, sizeof(de->source_file),
                                 calls[i].source_file);
                break;
            }
        }
        de->source_line = resources[r].open_line;
        snprintf(de->title, sizeof(de->title),
                 "Unclosed resource from %s()",
                 resources[r].open_func);
        snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                 "%s() at line %u was not closed with %s(). "
                 "Use a matching close call on every path.",
                 resources[r].open_func, resources[r].open_line,
                 resources[r].close_func);
        found++;
    }

    return found;
}

/* Module 5: free'd it twice (ouch) */

static int check_double_free(
    const CallSite *calls, int n_calls,
    DetectedError *errors, int max)
{
    int found = 0;

    for (int i = 0; i < n_calls && found < max; i++) {
        if (strcmp(calls[i].callee, "free") != 0 &&
            strcmp(calls[i].callee, "close") != 0 &&
            strcmp(calls[i].callee, "fclose") != 0)
            continue;

        /* Another call to the same function downstream? */
        for (int j = i + 1; j < n_calls && found < max; j++) {
            if (strcmp(calls[j].callee, calls[i].callee) != 0)
                continue;
            if (calls[j].line <= calls[i].line + 1)
                continue;

            /* Found a double call */
            DetectedError *de = &errors[found];
            memset(de, 0, sizeof(DetectedError));
            de->type = ERR_DOUBLE_FREE_CPP;
            de->severity = 3;
            de->has_source = true;
            copy_source_file(de->source_file, sizeof(de->source_file),
                             calls[j].source_file);
            de->source_line = calls[j].line;
            snprintf(de->title, sizeof(de->title),
                     "Double %s() at %s:%u",
                     calls[i].callee,
                     calls[j].source_file, calls[j].line);
            snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                     "%s() called multiple times. Remove the "
                     "duplicate or add a NULL/state check before "
                     "the second call.",
                     calls[i].callee);
            found++;
            break;  /* One report per initial call */
        }
    }

    return found;
}

/* Module 6: naughty — printf(var) instead of printf("%s", var) */

static int check_format_string(
    const CallSite *calls, int n_calls,
    AnnotationDB *ann, const char *source_path,
    DetectedError *errors, int max)
{
    int found = 0;

    for (int i = 0; i < n_calls && found < max; i++) {
        const Annotation *a = ann_lookup(ann, calls[i].callee);
        if (!a) continue;

        /* Does this function take a format string? */
        int fmt_param = -1;
        for (int p = 0; p < a->param_count; p++) {
            if (a->params[p].role == ROLE_FMT_STR) {
                fmt_param = p;
                break;
            }
        }
        if (fmt_param < 0) continue;

        /* Sniff the source to see if the format argument is a literal */
        FILE *fp = fopen(source_path, "r");
        if (!fp) continue;

        char line[2048];
        int lnum = 0;
        bool is_literal = false;
        bool found_line = false;
        while (fgets(line, sizeof(line), fp)) {
            lnum++;
            if (lnum != (int)calls[i].line) continue;
            found_line = true;

            const char *paren = strchr(line, '(');
            if (!paren) { is_literal = true; break; }
            paren++;

            /* Eat whitespace before the first arg */
            while (*paren == ' ' || *paren == '\t') paren++;

            /* Starts with " or is a constant — safe */
            if (*paren == '"') { is_literal = true; break; }

            if (strncmp(paren, "NULL", 4) == 0 ||
                strncmp(paren, "0", 1) == 0)
            { is_literal = true; break; }

            /* Variable format string — potential vuln */
            is_literal = false;
            break;
        }
        fclose(fp);

        if (is_literal || !found_line) continue;  /* no vuln */

        DetectedError *de = &errors[found];
        memset(de, 0, sizeof(DetectedError));
        de->type = ERR_BUFFER_OVERFLOW;
        de->severity = 2;
        de->has_source = true;
        copy_source_file(de->source_file, sizeof(de->source_file),
                         calls[i].source_file);
        de->source_line = calls[i].line;
        snprintf(de->title, sizeof(de->title),
                 "Format string vulnerability");
        snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                 "%s() with non-literal format string at %s:%u. "
                 "Use printf(\"%%s\", var) instead of printf(var).",
                 calls[i].callee, calls[i].source_file, calls[i].line);
        found++;
    }

    return found;
}

/* Module 7: atoi/atol result used as array index with no bounds check */

static int check_integer_overflow(
    const CallSite *calls, int n_calls,
    DetectedError *errors, int max)
{
    int found = 0;

    for (int i = 0; i < n_calls && found < max; i++) {
        if (strcmp(calls[i].callee, "atoi") != 0 &&
            strcmp(calls[i].callee, "atol") != 0)
            continue;

        DetectedError *de = &errors[found];
        memset(de, 0, sizeof(DetectedError));
        de->type = ERR_INT_OVERFLOW;
        de->severity = 2;
        de->has_source = true;
        copy_source_file(de->source_file, sizeof(de->source_file),
                         calls[i].source_file);
        de->source_line = calls[i].line;
        snprintf(de->title, sizeof(de->title),
                 "Unchecked string-to-integer conversion");
        snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                 "%s() at %s:%u returns int without error checking. "
                 "Use strtol() with errno validation for safe "
                 "conversion.",
                 calls[i].callee, calls[i].source_file, calls[i].line);
        found++;
    }

    return found;
}

/* Module 8: dereffed the ptr, THEN checked if it's NULL — lol */

static int check_null_order(
    const CallSite *calls, int n_calls,
    const char *source_path,
    DetectedError *errors, int max)
{
    int found = 0;

    for (int i = 0; i < n_calls && found < max; i++) {
        if (strcmp(calls[i].callee, "getenv") != 0 &&
            strcmp(calls[i].callee, "malloc") != 0 &&
            strcmp(calls[i].callee, "calloc") != 0 &&
            strcmp(calls[i].callee, "strdup") != 0)
            continue;

        /* Check the source file for a dereference after the call */
        FILE *fp = fopen(source_path, "r");
        if (!fp) continue;

        char line[2048];
        int lnum = 0;
        while (fgets(line, sizeof(line), fp)) {
            lnum++;
            if (lnum < (int)calls[i].line + 1) continue;
            if (lnum > (int)calls[i].line + 5) break;

            /* Sniff for -> or * dereference in the next few lines */
            if (strstr(line, "->") || strchr(line, '*')) {
                DetectedError *de = &errors[found];
                memset(de, 0, sizeof(DetectedError));
                de->type = ERR_NULL_DEREF;
                de->severity = 2;
                de->has_source = true;
                copy_source_file(de->source_file, sizeof(de->source_file),
                                 calls[i].source_file);
                de->source_line = calls[i].line;
                snprintf(de->title, sizeof(de->title),
                         "Potential NULL dereference of %s() result",
                         calls[i].callee);
                snprintf(de->fix_suggestion, sizeof(de->fix_suggestion),
                         "%s() can return NULL at %s:%u. The return "
                         "value is used at line %d without a prior "
                         "NULL check.",
                         calls[i].callee, calls[i].source_file,
                         calls[i].line, lnum);
                found++;
                break;
            }
        }
        fclose(fp);
    }

    return found;
}

/* Main entry: run all 8 modules */

int check_engine_run(ASTContext *ast, AnnotationDB *ann,
                     DetectedError *errors, int max_errors)
{
    if (!ast || !errors || max_errors <= 0)
        return 0;

    int total_errors = 0;

    /* Get function list to fish out the source file path */
    int n_funcs = ast_get_function_count(ast);
    FunctionInfo *funcs = NULL;
    if (n_funcs > 0) {
        funcs = (FunctionInfo *)malloc((size_t)n_funcs * sizeof(FunctionInfo));
        if (funcs)
            ast_get_functions(ast, funcs, n_funcs);
    }

    /* Collect all calls of interest into one flat array */
    CallSite all_calls[256];
    int n_all_calls = 0;
    static const char *key_funcs[] = {
        "malloc", "calloc", "realloc", "free", "strdup",
        "fopen", "fclose", "open", "close", "read", "write",
        "socket", "accept",
        "printf", "fprintf", "sprintf", "snprintf", "dprintf",
        "strcpy", "strncpy", "strcat", "strncat",
        "getenv", "atoi", "atol", "scanf",
        "pthread_mutex_lock", "pthread_mutex_unlock",
        NULL
    };

    for (int k = 0; key_funcs[k]; k++) {
        CallSite buf[32];
        int cnt = ast_find_calls_by_name(ast, key_funcs[k], buf, 32);
        for (int c = 0; c < cnt && n_all_calls < 256; c++)
            all_calls[n_all_calls++] = buf[c];
    }

    /* Sort by line — ordering checks depend on this */
    if (n_all_calls > 1)
        qsort(all_calls, n_all_calls, sizeof(CallSite), sort_by_line);

    /* Need the source path for line-level analysis */
    const char *source_path = NULL;
    if (funcs && n_funcs > 0)
        source_path = funcs[0].source_file;
    else if (n_all_calls > 0)
        source_path = all_calls[0].source_file;

    /*
     * Order matters here: modules that need funcs/source_path
     * run before funcs gets freed at the bottom.
     */

    /* Can run these without annotations or source_path */
    {
        DetectedError bufs[16];
        int cnt = check_resource_leak(all_calls, n_all_calls, ann, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }
    {
        DetectedError bufs[16];
        int cnt = check_double_free(all_calls, n_all_calls, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }
    {
        DetectedError bufs[16];
        int cnt = check_buffer_size(all_calls, n_all_calls, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }
    {
        DetectedError bufs[8];
        int cnt = check_integer_overflow(all_calls, n_all_calls, bufs, 8);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }

    /* Need the annotation DB */
    if (ann) {
        DetectedError bufs[16];
        int cnt = check_null_deref_in_func(all_calls, n_all_calls,
                                            ann, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }

    /* Need the source file path */
    if (source_path) {
        DetectedError bufs[16];
        int cnt = check_null_order(all_calls, n_all_calls,
                                    source_path, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }

    /* Need annotation DB for pairing checks */
    if (ann) {
        DetectedError bufs[16];
        int cnt = check_uaf_in_func(all_calls, n_all_calls,
                                     ann, bufs, 16);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }

    /* Need both source file and annotations */
    if (source_path && ann) {
        DetectedError bufs[8];
        int cnt = check_format_string(all_calls, n_all_calls,
                                       ann, source_path, bufs, 8);
        for (int i = 0; i < cnt && total_errors < max_errors; i++)
            errors[total_errors++] = bufs[i];
    }

    free(funcs);
    return total_errors;
}
