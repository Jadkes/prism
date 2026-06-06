/*
 * check_engine.h - Static analysis check modules for c_tester
 *
 * Purpose: 8 built-in check modules that use the AST backend (Phase 1)
 *          and annotation system (Phase 2) to detect flow-aware bugs.
 *          Each module is a focused detector that looks for specific
 *          bug patterns during a single AST traversal.
 *
 * Checks:
 *   1. Null-deref path    — ptr=NULL then ptr->field
 *   2. Use-after-free     — free(ptr) then ptr->field
 *   3. Buffer-size        — strcpy/sprintf without size limit
 *   4. Resource leak      — fopen without fclose
 *   5. Double-free/close  — free(ptr) then free(ptr) again
 *   6. Format string      — printf(user_input) without format
 *   7. Integer overflow   — atoi(user) used as buffer index unchecked
 *   8. Null-check order   — ptr->field before if(!ptr)
 *
 * Design: Single-pass traversal of each function's cursor tree in the
 *         AST context. Checks run inline during traversal against a
 *         per-function variable/resource tracking table.
 *
 * Thread-safety: Single-threaded. Each run is independent.
 */

#ifndef CHECK_ENGINE_H
#define CHECK_ENGINE_H

#include "c_tester.h"
#include "ast_backend.h"
#include "annotation.h"

/*
 * check_engine_run - Run all 8 check modules against an AST + annotations
 *
 * Walks each function's cursor tree and applies every check module.
 * Results are accumulated into errors[] via merge_analysis_error.
 *
 * @param ast - Parsed AST context (from ast_parse)
 * @param ann - Annotation database (from ann_load, or NULL)
 * @param errors - Output array for detected errors
 * @param max_errors - Capacity of errors[]
 * @return Number of errors found
 */
int check_engine_run(ASTContext *ast, AnnotationDB *ann,
                     DetectedError *errors, int max_errors);

#endif /* CHECK_ENGINE_H */
