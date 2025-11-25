#include "sem_analysis.h"
#include "err.h"
#include <stdlib.h>
#include <string.h>

static SemContext *sem_ctx_create(void);
static void sem_ctx_free(SemContext *ctx);
static void sem_enter_scope(SemContext *ctx);
static void sem_leave_scope(SemContext *ctx);

static bool sem_register_functions(SemContext *ctx, ASTNode *root);
static bool sem_visit(SemContext *ctx, ASTNode *Node);

static bool sem_function_def(SemContext *ctx, ASTNode *def);
static bool sem_normal_function(SemContext *ctx,
                                const char *name,
                                ASTNode *func_node);
static bool sem_getter(SemContext *ctx,
                       const char *name,
                       ASTNode *getter_node);
static bool sem_setter(SemContext *ctx,
                       const char *name,
                       ASTNode *setter_node);

// ---------------------------
// Context management
// ---------------------------
static SemContext *sem_ctx_create(void) {
    SemContext *ctx = calloc(1, sizeof(SemContext));
    if (!ctx) error_exit(99, "Out of memory (SemContext)\n");

    ctx->global_scope  = symtable_create(NULL);
    ctx->current_scope = ctx->global_scope;
    ctx->has_main_noargs = false;

    return ctx;
}

void sem_free(SemContext *ctx)
{
    if (!ctx) return;
    symtable_free(ctx->global_scope);
    free(ctx);
}

static void enter_scope(SemContext *ctx)
{
    SymTable *new_scope = symtable_create(ctx->current_scope);
    if (!new_scope) error_exit(99, "Out of memory (scope)\n");
    ctx->current_scope = new_scope;
}

static void leave_scope(SemContext *ctx)
{
    SymTable *parent = ctx->current_scope->prev;
    symtable_free(ctx->current_scope);
    ctx->current_scope = parent;
}

// ---------------------------
// Driver
// ---------------------------
bool sem_analyze(ASTNode *root)
{
    SemContext *ctx = sem_create();

    if(!sem_register_functions(ctx, root)) {
        sem_free(ctx);
        return false;
    }

    bool ok = sem_visit(ctx, root);

    if (!ctx->has_main_noargs)
    {
        error_exit(3, "Semantic error: missing main() with no Parameters\n");
        ok = false;
    }

    sem_free(ctx);
    return ok;
}

// ---------------------------
// Dispatcher
// ---------------------------

static bool sem_register_functions(SemContext *ctx, ASTNode *Node) {
    if (!Node) return true;

    if (Node->type == AST_FUNCTION_DEF) {
        if (!sem_function_def(ctx, Node))
            return false;
    }

    for (int i = 0; i < Node->child_count; ++i) {
        if (!sem_register_functions(ctx, Node->children[i]))
            return false;
    }
    return true;
}

static bool sem_visit(SemContext *ctx, ASTNode *Node) {
    if (!Node) return true;

    switch (Node->type) {
        case AST_FUNCTION_DEF:
            // second pass: only body & Param (signature already inserted)
        {
            ASTNode *Name_Node = Node->children[0];
            ASTNode *Kind_Node = Node->children[1];
            const char *Name = Name_Node->token->lexeme;

            switch (Kind_Node->type) {
                case AST_FUNCTION:
                    return sem_normal_function(ctx, Name, Kind_Node);
                case AST_GETTER:
                    return sem_getter(ctx, Name, Kind_Node);
                case AST_SETTER:
                    return sem_setter(ctx, Name, Kind_Node);
                default:
                    error_exit(99, "Internal error: bad function kind\n");
            }
        }

        // later: cases for AST_BLOCK, AST_VAR_DECL, AST_ASSIGN, ...

        default:
            for (int i = 0; i < Node->child_count; ++i)
                if (!sem_visit(ctx, Node->children[i]))
                    return false;
            return true;
    }
}


// ---------------------------
// Semantic handlers
// ---------------------------

static bool sem_function_def(SemContext *ctx, ASTNode *def) {
    if (def->child_count != 2) {
        error_exit(99, "Internal error: bad FUNCTION_DEF arity\n");
    }

    ASTNode *Name_Node = def->children[0];
    ASTNode *Kind_Node = def->children[1];

    const char *Name = Name_Node->token->lexeme;

    if (!Name || Name_Node->type != AST_IDENTIFIER) {
        error_exit(99, "Internal error: function name missing\n");
    }

    // normal function / getter / setter?
    if (Kind_Node->type == AST_FUNCTION) {
        // assume FUNCTION children[0] = Param_LIST, children[1] = BLOCK
        ASTNode *Param = Kind_Node->children[0];
        int arity = Param ? Param->child_count : 0;

        // name + arity overloading key
        char *key = make_func_key(Name, arity);
        SymInfo *existing = symtable_find(ctx->global_scope, key);
        if (existing) {
            // already a function with same name+arity → error 4
            error_exit(4,
                       "Semantic error: redefinition of function %s with %d Param\n",
                       Name, arity);
        }

        SymInfo *Sym = calloc(1, sizeof(SymInfo));
        if (!Sym) error_exit(99, "Out of memory (SymInfo func)\n");

        Sym->kind = SYM_FUNC;
        Sym->info.func.arity = arity;
        Sym->info.func.param_type_mask = NULL;   // filled in by type analysis
        Sym->info.func.ret_type_mask   = TYPEMASK_ALL;
        Sym->info.func.declared        = true;
        Sym->info.func.defined         = true;
        Sym->info.func.is_getter       = false;
        Sym->info.func.is_setter       = false;

        if (!symtable_insert(ctx->global_scope, key, Sym)) {
            error_exit(99, "Internal error: symtable_insert failed\n");
        }

        // main() with no args
        ctx->has_main_noargs |=
            (strcmp(Name, "main") == 0 && arity == 0);

        free(key);
        return true;
    }
    else if (Kind_Node->type == AST_GETTER) {
        return sem_getter(ctx, Name, Kind_Node);
    }
    else if (Kind_Node->type == AST_SETTER) {
        return sem_setter(ctx, Name, Kind_Node);
    }

    error_exit(99, "Internal error: unknown function kind\n");
}

static bool sem_getter(SemContext *ctx,
                       const char *Name,
                       ASTNode *Getter_Node)
{
    // getter key independent of arity
    char *Key = make_func_key(Name, 0);
    SymInfo *existing = symtable_find(ctx->global_scope, Key);

    if (existing) {
        if (existing->kind == SYM_FUNC &&
            existing->info.func.is_getter) {
            error_exit(4,
                       "Semantic error: duplicate static getter '%s'\n",
                       Name);
        } else {
            error_exit(4,
                       "Semantic error: '%s' already used for a different symbol\n",
                       Name);
        }
    }

    // insert getter symbol
    SymInfo *Sym = calloc(1, sizeof(SymInfo));
    if (!Sym) error_exit(99, "Out of memory (getter SymInfo)\n");

    Sym->kind = SYM_FUNC;
    Sym->info.func.arity = 0;
    Sym->info.func.param_type_mask = NULL;
    Sym->info.func.ret_type_mask   = TYPEMASK_ALL;
    Sym->info.func.declared        = true;
    Sym->info.func.defined         = true;
    Sym->info.func.is_getter       = true;
    Sym->info.func.is_setter       = false;

    if (!symtable_insert(ctx->global_scope, Key, Sym)) {
        error_exit(99, "Internal error: symtable_insert(getter) failed\n");
    }
    free(Key);

    // second-pass: visit body
    // getter has children[0] = BLOCK
    sem_enter_scope(ctx);
    ASTNode *body = Getter_Node->children[0];
    if (!sem_visit(ctx, body)) {
        sem_leave_scope(ctx);
        return false;
    }
    sem_leave_scope(ctx);
    return true;
}

static bool sem_setter(SemContext *ctx,
                       const char *name,
                       ASTNode *Setter_Node)
{
    char *key = make_setter_key(name);
    SymInfo *existing = symtable_find(ctx->global_scope, key);

    if (existing) {
        if (existing->kind == SYM_FUNC &&
            existing->info.func.is_setter) {
            error_exit(4,
                       "Semantic error: duplicate static setter '%s'\n",
                       name);
        } else {
            error_exit(4,
                       "Semantic error: '%s' already used for a different symbol\n",
                       name);
        }
    }

    // insert setter symbol
    SymInfo *Sym = calloc(1, sizeof(SymInfo));
    if (!Sym) error_exit(99, "Out of memory (setter SymInfo)\n");

    Sym->kind = SYM_FUNC;
    Sym->info.func.arity = 1;
    Sym->info.func.param_type_mask = NULL;
    Sym->info.func.ret_type_mask   = TYPEMASK_ALL;
    Sym->info.func.declared        = true;
    Sym->info.func.defined         = true;
    Sym->info.func.is_getter       = false;
    Sym->info.func.is_setter       = true;

    if (!symtable_insert(ctx->global_scope, key, Sym)) {
        error_exit(99, "Internal error: symtable_insert(setter) failed\n");
    }
    free(key);

    // second-pass: create scope with param as local
    sem_enter_scope(ctx);

    ASTNode *Param_Node = Setter_Node->children[0];
    ASTNode *body = Setter_Node->children[1];

    // insert parameter as local variable symbol
    SymInfo *Param_Sym = calloc(1, sizeof(SymInfo));
    if (!Param_Sym) error_exit(99, "Out of memory (setter param)\n");

    Param_Sym->kind = SYM_VAR;
    Param_Sym->info.var.is_global  = false;
    Param_Sym->info.var.type_mask  = TYPEMASK_ALL;

    if (!symtable_insert(ctx->current_scope,
                         Param_Node->token->lexeme,
                         Param_Sym)) {
        error_exit(4,
                   "Semantic error: duplicate parameter '%s' in setter '%s'\n",
                   Param_Node->token->lexeme,
                   name);
    }

    if (!sem_visit(ctx, body)) {
        sem_leave_scope(ctx);
        return false;
    }

    sem_leave_scope(ctx);
    return true;
}

static bool sem_normal_function(SemContext *ctx,
                                const char *Name,
                                ASTNode *Func_node)
{
    // FUNCTION: children[0] = PARAM_LIST, children[1] = BLOCK
    ASTNode *Params = Func_node->children[0];
    ASTNode *Body   = Func_node->children[1];

    sem_enter_scope(ctx);

    // insert parameters as locals
    for (int i = 0; Params && i < Params->child_count; ++i) {
        ASTNode *Param_ID = Params->children[i];

        SymInfo *fun_sym = calloc(1, sizeof(SymInfo));
        if (!fun_sym) error_exit(99, "Out of memory (func param)\n");

        fun_sym->kind = SYM_VAR;
        fun_sym->info.var.is_global = false;
        fun_sym->info.var.type_mask = TYPEMASK_ALL;

        if (!symtable_insert(ctx->current_scope,
                             Param_ID->token->lexeme,
                             fun_sym)) {
            error_exit(4,
                       "Semantic error: duplicate parameter '%s' in function '%s'\n",
                       Param_ID->token->lexeme,
                       Name);
        }
    }

    // visit body statements
    bool ok = sem_visit(ctx, Body);

    sem_leave_scope(ctx);
    return ok;
}

static bool sem_block(SemContext *ctx, ASTNode *Node)
{
    enter_scope(ctx);

    for (int i = 0; i < Node->child_count; i++)
        if (!sem_visit(ctx, Node->children[i]))
            return false;

    leave_scope(ctx);
    return true;
}

static bool sem_var_decl(SemContext *ctx, ASTNode *Node)
{
    // TODO:
    // - ensure variable is not already declared in this scope
    // - insert symbol as SYM_VAR
    return true;
}

static bool sem_assign(SemContext *ctx, ASTNode *Node)
{
    

    

    return sem_expression(ctx, Node->children[1]);
}

static bool sem_call(SemContext *ctx, ASTNode *Node)
{
    // TODO:
    // - check function existence
    // - check arity
    // - check that Param are terms, not expressions (unless FUNEXP implemented)
    return true;
}

static bool sem_expression(SemContext *ctx, ASTNode *Node)
{
    // semantic layer does NOT do type inference
    // only checks existence of identifiers (getters allowed)
    return true;
}
