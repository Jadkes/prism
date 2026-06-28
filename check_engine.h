/*
 * check_engine.h - 8 static analysis check modules
 *
 * Uses AST call-site data and function annotations to find flow-aware bugs:
 *   - null deref paths, use-after-free, unbounded strcpy, resource leaks
 *   - double-free, printf-var format strings, unchecked atoi, null-order
 */

#ifndef CHECK_ENGINE_H
#define CHECK_ENGINE_H

#include "prism.h"
#include "ast_backend.h"
#include "annotation.h"

/* Run all 8 check modules against an AST + annotations */
int check_engine_run(ASTContext *ast, AnnotationDB *ann,
                     DetectedError *errors, int max_errors);

#endif /* CHECK_ENGINE_H */
