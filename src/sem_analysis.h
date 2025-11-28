#ifndef SEM_ANALYSIS_H
#define SEM_ANALYSIS_H

#include "ast.h"
#include "err.h"
#include "symtable.h"

typedef struct FuncRecord {
    SymInfo           *sym;
    struct FuncRecord *next;
} FuncRecord;

typedef struct {
    SymTable *global_scope;
    SymTable *current_scope;

    bool has_main_noargs;
    FuncRecord *func_list;        // list of all user functions (for final check)
} SemContext;

SemContext *sem_create();
void sem_free(SemContext *ctx);

/// Run semantic analysis on full AST.
/// Returns true if OK; false on semantic error.
bool sem_analyze(ASTNode *root);

#endif