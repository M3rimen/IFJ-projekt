#ifndef SEM_ANALYSIS_H
#define SEM_ANALYSIS_H

#include "ast.h"
#include "symtable.h"

typedef struct {
    SymTable *global_scope;
    SymTable *current_scope;

    // track main() existence
    bool has_main_noargs;
} SemContext;

SemContext *sem_create();
void sem_free(SemContext *ctx);

/// Run semantic analysis on full AST.
/// Returns true if OK; false on semantic error.
bool sem_analyze(ASTNode *root);

#endif