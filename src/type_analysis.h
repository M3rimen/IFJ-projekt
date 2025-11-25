#ifndef TYPE_ANALYSIS_H
#define TYPE_ANALYSIS_H

#include "ast.h"
#include "symtable.h"

typedef struct {
    SymTable *global_scope;
    SymTable *current_scope;
} TypeContext;

/// Run full type inference + static type error detection.
/// Returns true on success, false on type error.
bool type_analyze(ASTNode *root, SymTable *global_symtable);

#endif
