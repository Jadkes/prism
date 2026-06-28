/*
 * sym_exec.c - Lightweight symbolic execution engine
 *
 * Scans source lines for if/while/for conditions, forks state at each
 * branch, tracks integer ranges. 512 paths max, 10 blocks deep.
 * check_engine calls in to ask about variable ranges on feasible paths.
 */

#include "sym_exec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

/* Helpers: pulling conditions out of source lines */

/* Grab a variable name from the start of a C expression */
static int extract_var_name(const char *src, char *out, int out_size)
{
    int i = 0;
    while (*src && i < out_size - 1) {
        if (isalnum((unsigned char)*src) || *src == '_') {
            out[i++] = *src++;
        } else {
            break;
        }
    }
    out[i] = '\0';
    return i;
}

/* Parse "i < 10" or "x >= 0" from a condition string */
static bool parse_comparison(const char *cond_text,
                              char *var_out, int var_size,
                              SymCmpOp *op_out, int *val_out)
{
    if (!cond_text || !*cond_text)
        return false;

    /* Skip leading paren if any */
    while (*cond_text == '(' || *cond_text == ' ')
        cond_text++;

    /* Try to extract a variable name */
    char vname[64];
    if (extract_var_name(cond_text, vname, sizeof(vname)) == 0)
        return false;

    /* Advance past variable name in the source */
    cond_text += strlen(vname);

    /* Eat whitespace then look for comparison op */
    SymCmpOp op;
    if (strncmp(cond_text, "<=", 2) == 0) {
        op = SYM_CMP_LE;
        cond_text += 2;
    } else if (strncmp(cond_text, ">=", 2) == 0) {
        op = SYM_CMP_GE;
        cond_text += 2;
    } else if (strncmp(cond_text, "==", 2) == 0) {
        op = SYM_CMP_EQ;
        cond_text += 2;
    } else if (strncmp(cond_text, "!=", 2) == 0) {
        op = SYM_CMP_NE;
        cond_text += 2;
    } else if (*cond_text == '<') {
        op = SYM_CMP_LT;
        cond_text++;
    } else if (*cond_text == '>') {
        op = SYM_CMP_GT;
        cond_text++;
    } else if (*cond_text == '=') {
        /* '=' is assignment, not comparison — skip */
        return false;
    } else {
        return false;
    }

    /* Skip whitespace after operator */
    while (*cond_text == ' ') cond_text++;

    /* Is the right side a number? */
    if (*cond_text == '-' || isdigit((unsigned char)*cond_text)) {
        char *end = NULL;
        long v = strtol(cond_text, &end, 10);
        if (end > cond_text) {
            strncpy(var_out, vname, (size_t)var_size - 1);
            var_out[var_size - 1] = '\0';
            *op_out = op;
            *val_out = (int)v;
            return true;
        }
    }

    return false;
}

/* Branch point: an if/while/for with a parsed condition */
typedef struct {
    int line_number;
    char condition[256];
    int true_target;   /* line number of first non-branch line (true side) */
    int false_target;  /* line number of first line after the block (false) */
} BranchPoint;

/* Cap on branch points we track */
#define MAX_BRANCHES 64

/* SymState operations */

/* Allocate a new empty state; NULL on failure */
static SymState *sym_state_new(void)
{
    SymState *s = (SymState *)calloc(1, sizeof(SymState));
    if (s) {
        s->var_count = 0;
        s->depth = 0;
        s->next = NULL;
    }
    return s;
}

/* Deep-copy a SymState */
static SymState *sym_state_clone(const SymState *src)
{
    if (!src) return NULL;
    SymState *s = sym_state_new();
    if (!s) return NULL;
    *s = *src;       /* shallow-copy fields, don't link into the list */
    s->next = NULL;  /* SymVarRange entries are value types — clean copy */
    return s;
}

/* Find or create a variable range entry */
static SymVarRange *sym_get_var(SymState *s, const char *name)
{
    for (int i = 0; i < s->var_count; i++) {
        if (strcmp(s->vars[i].var_name, name) == 0)
            return &s->vars[i];
    }
    /* Not found yet — create it */
    if (s->var_count >= 64) return NULL;
    SymVarRange *vr = &s->vars[s->var_count++];
    memset(vr, 0, sizeof(SymVarRange));
    strncpy(vr->var_name, name, sizeof(vr->var_name) - 1);
    vr->min_val = INT_MIN;
    vr->max_val = INT_MAX;
    vr->alloc_size = -1;
    vr->is_null = false;
    vr->is_nonnull = false;
    return vr;
}

/* Narrow a variable's range with a constraint; false if infeasible */
static bool sym_apply_constraint(SymState *s,
                                  const char *var,
                                  SymCmpOp op, int val)
{
    SymVarRange *vr = sym_get_var(s, var);
    if (!vr) return false;   /* out of slots — conservatively assume feasible */

    switch (op) {
    case SYM_CMP_LT: if (val - 1 < vr->max_val) vr->max_val = val - 1; break;
    case SYM_CMP_LE: if (val < vr->max_val)     vr->max_val = val;     break;
    case SYM_CMP_GT: if (val + 1 > vr->min_val) vr->min_val = val + 1; break;
    case SYM_CMP_GE: if (val > vr->min_val)     vr->min_val = val;     break;
    case SYM_CMP_EQ: vr->min_val = vr->max_val = val; break;
    case SYM_CMP_NE:
        /* i != 5: split into i <= 4 OR i >= 6 — but we can't fork here.
         * Instead, just narrow one end if it's at the boundary. */
        if (vr->min_val == val) vr->min_val = val + 1;
        if (vr->max_val == val) vr->max_val = val - 1;
        break;
    }

    /* Infeasible if min > max */
    if (vr->min_val > vr->max_val) return false;

    /* Pointer-specific tracking (NULL / non-NULL) */
    if (op == SYM_CMP_EQ) {
        if (val == 0) {
            vr->is_null = true;
            vr->is_nonnull = false;
        } else {
            vr->is_null = false;
            vr->is_nonnull = true;
        }
    } else if (op == SYM_CMP_NE && val == 0) {
        vr->is_null = false;
        vr->is_nonnull = true;
    }

    return true;
}

/* Branch detection — scans source lines for if/while/for */
static int find_branches(const char *source_path,
                          BranchPoint *branches, int max_branches)
{
    FILE *fp = fopen(source_path, "r");
    if (!fp) return 0;

    char line[2048];
    int lnum = 0;
    int nb = 0;

    while (fgets(line, sizeof(line), fp)) {
        lnum++;
        if (nb >= max_branches) break;

        /* Look for if ( / while ( / for ( */
        const char *cond_start = NULL;
        bool is_for = false;

        const char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (strncmp(trimmed, "if (", 4) == 0) {
            cond_start = trimmed + 4;
        } else if (strncmp(trimmed, "while (", 7) == 0) {
            cond_start = trimmed + 7;
        } else if (strncmp(trimmed, "for (", 5) == 0) {
            cond_start = trimmed + 5;
            is_for = true;
        }

        if (!cond_start) continue;

        /* Pull out the text between the outer parens */
        char cond_text[256] = {0};
        int depth = 0;
        int ci = 0;
        const char *p = cond_start;

        while (*p && ci < (int)sizeof(cond_text) - 1) {
            if (*p == '(') depth++;
            else if (*p == ')') {
                if (depth == 0) break;
                depth--;
            }

            /* For for-loops, grab the middle expression (the actual condition) */
            if (is_for) {
                /* Skip the init expression — stop at first semicolon */
                if (ci == 0 && *p == ';') {
                    cond_text[0] = '\0';
                    ci = 0;
                    p++;
                    /* Now reading the condition */
                    while (*p && *p != ';') {
                        if (ci < (int)sizeof(cond_text) - 1)
                            cond_text[ci++] = *p;
                        p++;
                    }
                    cond_text[ci] = '\0';
                    break;
                }
                /* Still in the init part — keep skipping */
                p++;
                continue;
            }

            cond_text[ci++] = *p;
            p++;
        }
        cond_text[ci] = '\0';

        if (cond_text[0]) {
            branches[nb].line_number = lnum;
            snprintf(branches[nb].condition,
                     sizeof(branches[nb].condition), "%.*s",
                     (int)sizeof(branches[nb].condition) - 1,
                     cond_text);
            branches[nb].true_target = lnum + 1;   /* approximate */
            branches[nb].false_target = lnum + 5;   /* approximate */
            nb++;
        }
    }

    fclose(fp);
    return nb;
}

/* Path set: fork every path at a branch, prune infeasible ones */
static void sym_fork_at_branch(SymPathSet *ps,
                                 const char *var,
                                 SymCmpOp op, int val)
{
    int old_count = ps->path_count;
    for (int i = 0; i < old_count && ps->path_count < ps->max_paths; i++) {
        SymState *original = ps->paths;
        if (!original) break;
        ps->paths = original->next;
        original->next = NULL;
        ps->path_count--;

        /* Fork: true branch — condition holds */
        SymState *true_state = sym_state_clone(original);
        if (true_state) {
            true_state->depth = original->depth + 1;
            if (sym_apply_constraint(true_state, var, op, val)) {
                /* Append to the path list */
                SymState **p = &ps->paths;
                while (*p) p = &(*p)->next;
                *p = true_state;
                ps->path_count++;
            } else {
                free(true_state);
            }
        }

        /* Fork: false branch — negated condition */
        SymState *false_state = sym_state_clone(original);
        if (false_state) {
            false_state->depth = original->depth + 1;
            /* Negate the comparison */
            SymCmpOp neg_op;
            switch (op) {
            case SYM_CMP_LT: neg_op = SYM_CMP_GE; break;
            case SYM_CMP_LE: neg_op = SYM_CMP_GT; break;
            case SYM_CMP_GT: neg_op = SYM_CMP_LE; break;
            case SYM_CMP_GE: neg_op = SYM_CMP_LT; break;
            case SYM_CMP_EQ: neg_op = SYM_CMP_NE; break;
            case SYM_CMP_NE: neg_op = SYM_CMP_EQ; break;
            default: neg_op = SYM_CMP_NE; break;
            }
            if (sym_apply_constraint(false_state, var, neg_op, val)) {
                SymState **p = &ps->paths;
                while (*p) p = &(*p)->next;
                *p = false_state;
                ps->path_count++;
            } else {
                free(false_state);
            }
        }

        free(original);
    }
}

/* Public API */

SymPathSet sym_analyze_source(const char *source_path,
                               const char *func_name,
                               unsigned func_line,
                               int max_paths)
{
    (void)func_name;
    (void)func_line;

    SymPathSet result;
    memset(&result, 0, sizeof(result));
    result.max_paths = (max_paths > 0) ? max_paths : 512;

    if (!source_path) return result;

    /* Find all branch conditions in the source */
    BranchPoint branches[MAX_BRANCHES];
    int nb = find_branches(source_path, branches, MAX_BRANCHES);

    if (nb == 0) {
        /* No branches: single path, empty constraints */
        result.paths = sym_state_new();
        result.path_count = 1;
        return result;
    }

    /* Bootstrap with one empty path */
    result.paths = sym_state_new();
    result.path_count = 1;

    /* Fork at each branch */
    for (int b = 0; b < nb; b++) {
        if (result.path_count >= result.max_paths) break;
        if (result.paths == NULL) break;

        /* Parse and apply the condition */
        char var[64];
        SymCmpOp op;
        int val;

        if (parse_comparison(branches[b].condition,
                              var, sizeof(var), &op, &val)) {
            sym_fork_at_branch(&result, var, op, val);
        }
        /* Can't parse? Keep all paths — conservative: don't prune */
    }

    return result;
}

void sym_free_paths(SymPathSet *ps)
{
    if (!ps) return;
    SymState *s = ps->paths;
    while (s) {
        SymState *next = s->next;
        free(s);
        s = next;
    }
    ps->paths = NULL;
    ps->path_count = 0;
}

bool sym_can_be_negative(const SymPathSet *ps, const char *var_name)
{
    if (!ps || !var_name || !ps->paths)
        return true;   /* can't check? assume yes */

    SymState *s = ps->paths;
    while (s) {
        for (int v = 0; v < s->var_count; v++) {
            if (strcmp(s->vars[v].var_name, var_name) == 0) {
                if (s->vars[v].min_val < 0)
                    return true;   /* found a path where var can be < 0 */
                break;
            }
        }
        s = s->next;
    }

    return false;   /* no path lets this go negative */
}

bool sym_can_exceed(const SymPathSet *ps, const char *var_name, int bound)
{
    if (!ps || !var_name || !ps->paths)
        return true;   /* assume yes if we can't check */

    SymState *s = ps->paths;
    while (s) {
        for (int v = 0; v < s->var_count; v++) {
            if (strcmp(s->vars[v].var_name, var_name) == 0) {
                if (s->vars[v].max_val > bound)
                    return true;
                break;
            }
        }
        s = s->next;
    }

    return false;
}

bool sym_is_always_nonnull(const SymPathSet *ps, const char *var_name)
{
    if (!ps || !var_name || !ps->paths)
        return false;   /* assume no if we can't check */

    SymState *s = ps->paths;
    while (s) {
        for (int v = 0; v < s->var_count; v++) {
            if (strcmp(s->vars[v].var_name, var_name) == 0) {
                if (s->vars[v].is_null || !s->vars[v].is_nonnull)
                    return false;  /* path allows null */
                break;
            }
        }
        s = s->next;
    }

    /* Every feasible path says this ptr is non-NULL */
    return true;
}

int sym_get_alloc_size(const SymPathSet *ps, const char *var_name)
{
    if (!ps || !var_name)
        return -1;

    SymState *s = ps->paths;
    while (s) {
        for (int v = 0; v < s->var_count; v++) {
            if (strcmp(s->vars[v].var_name, var_name) == 0)
                return s->vars[v].alloc_size;
        }
        s = s->next;
    }
    return -1;
}
