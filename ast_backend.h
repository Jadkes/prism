/*
 * ast_backend.h - libclang AST wrapper
 *
 * Opaque ASTContext carries a parsed CXTranslationUnit. One-pass cursor
 * traversal at parse time collects everything; query functions return
 * pre-populated arrays so repeated lookups are O(1).
 */

#ifndef AST_BACKEND_H
#define AST_BACKEND_H

#include "prism.h"

/* AST query output types */

/* A dangerous function call found in the AST cursor tree */
typedef struct {
    char function_name[128];
    char source_file[MAX_PATH_LEN];
    unsigned line;
    unsigned column;
    int severity;
    ErrorType type;
} DangerousCall;

/* Info about a function definition */
typedef struct {
    char name[128];
    char source_file[MAX_PATH_LEN];
    unsigned line;
    unsigned column;
    unsigned param_count;
    bool returns_pointer;
} FunctionInfo;

/* A function call site (caller → callee) */
typedef struct {
    char callee[128];
    char caller[128];
    char source_file[MAX_PATH_LEN];
    unsigned line;
    unsigned column;
} CallSite;

/* Opaque context — allocated by ast_parse, freed by ast_free */
typedef struct ASTContext ASTContext;

/* AST Backend API */

/* Parse a C source into an AST context (caller owns, must ast_free) */
ASTContext *ast_parse(const char *source_file, const char **compiler_args);

/*
 * ast_free - Free all resources held by an AST context
 */
void ast_free(ASTContext *ctx);

/* Get all dangerous API calls found during parse (filtered to user source) */
int ast_get_dangerous_calls(ASTContext *ctx,
                            DangerousCall *out, int max);

/*
 * ast_get_function_count - Number of function definitions found
 */
int ast_get_function_count(ASTContext *ctx);

/* Get function definition info */
int ast_get_functions(ASTContext *ctx, FunctionInfo *out, int max);

/* Find all call sites of a named function (for pair tracking, etc.) */
int ast_find_calls_by_name(ASTContext *ctx, const char *func_name,
                           CallSite *out, int max);

#endif /* AST_BACKEND_H */
