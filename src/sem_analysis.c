#include "sem_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------
// Internal forward declarations
// ---------------------------
static void enter_scope(SemContext *ctx);
static void leave_scope(SemContext *ctx);

static bool sem_visit(SemContext *ctx, ASTNode *node);
static bool sem_function(SemContext *ctx, ASTNode *node);
static bool sem_block(SemContext *ctx, ASTNode *node);
static bool sem_statement(SemContext *ctx, ASTNode *node);
static bool sem_expression(SemContext *ctx, ASTNode *node);
static bool sem_var_decl(SemContext *ctx, ASTNode *node);
static bool sem_assign(SemContext *ctx, ASTNode *node);
static bool sem_call(SemContext *ctx, ASTNode *node);

// ---------------------------
// Context management
// ---------------------------
SemContext *sem_create() {
    SemContext *ctx = calloc(1, sizeof(SemContext));
    ctx->global_scope = symtable_create(NULL);
    ctx->current_scope = ctx->global_scope;
    return ctx;
}

void sem_free(SemContext *ctx) {
    symtable_free(ctx->global_scope);
    free(ctx);
}

static void enter_scope(SemContext *ctx) {
    SymTable *new_scope = symtable_create(ctx->current_scope);
    ctx->current_scope = new_scope;
}

static void leave_scope(SemContext *ctx) {
    SymTable *parent = ctx->current_scope->prev;
    symtable_free(ctx->current_scope);
    ctx->current_scope = parent;
}

// ---------------------------
// Driver
// ---------------------------
bool sem_analyze(ASTNode *root) {
    SemContext *ctx = sem_create();

    bool ok = sem_visit(ctx, root);

    if (!ctx->has_main_noargs) {
        fprintf(stderr, "Semantic error: missing main() with no parameters.\n");
        ok = false;
    }

    sem_free(ctx);
    return ok;
}

// ---------------------------
// Dispatcher
// ---------------------------
static bool sem_visit(SemContext *ctx, ASTNode *node) {
    switch (node->type) {

        case AST_FUNCTION:
            return sem_function(ctx, node);

        case AST_BLOCK:
            return sem_block(ctx, node);

        case AST_VAR_DECL:
            return sem_var_decl(ctx, node);

        case AST_ASSIGN:
            return sem_assign(ctx, node);

        case AST_CALL:
            return sem_call(ctx, node);

        case AST_EXPR:
            return sem_expression(ctx, node);

        default:
            // Generic traversal for other node types
            for (int i = 0; i < node->child_count; i++)
                if (!sem_visit(ctx, node->children[i]))
                    return false;
            return true;
    }
}

// ---------------------------
// Semantic handlers
// ---------------------------

static bool sem_function(SemContext *ctx, ASTNode *node) {
    // TODO:
    // - extract identifier and params
    // - check duplicate definitions
    // - insert into symbol table
    // - set ctx->has_main_noargs if needed

    // enter function scope
    enter_scope(ctx);

    // process function body
    for (int i = 0; i < node->child_count; i++)
        if (!sem_visit(ctx, node->children[i]))
            return false;

    leave_scope(ctx);
    return true;
}

static bool sem_block(SemContext *ctx, ASTNode *node) {
    enter_scope(ctx);

    for (int i = 0; i < node->child_count; i++)
        if (!sem_visit(ctx, node->children[i]))
            return false;

    leave_scope(ctx);
    return true;
}

static bool sem_var_decl(SemContext *ctx, ASTNode *node) {
    // TODO:
    // - ensure variable is not already declared in this scope
    // - insert symbol as SYM_VAR
    return true;
}

static bool sem_assign(SemContext *ctx, ASTNode *node) {
    // TODO:
    // - ensure variable exists or is global
    // - ensure setter rules if identifier is a setter
    return sem_expression(ctx, node->children[1]);
}

static bool sem_call(SemContext *ctx, ASTNode *node) {
    // TODO:
    // - check function existence
    // - check arity
    // - check that params are terms, not expressions (unless FUNEXP implemented)
    return true;
}

static bool sem_expression(SemContext *ctx, ASTNode *node) {
    // semantic layer does NOT do type inference
    // only checks existence of identifiers (getters allowed)
    return true;
}
