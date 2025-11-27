#include "sem_analysis.h"
#include "err.h"
#include <stdlib.h>
#include <string.h>

/* Forward decls */
static SemContext *sem_ctx_create(void);
static void sem_ctx_free(SemContext *ctx);
static void sem_enter_scope(SemContext *ctx);
static void sem_leave_scope(SemContext *ctx);

static bool sem_register_functions(SemContext *ctx, ASTNode *root);
static bool sem_visit(SemContext *ctx, ASTNode *node);

/* per-node handlers */
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

static bool sem_block(SemContext *ctx, ASTNode *node);
static bool sem_var_decl(SemContext *ctx, ASTNode *node);
static bool sem_assign(SemContext *ctx, ASTNode *node);
static bool sem_call(SemContext *ctx, ASTNode *node);
static bool sem_expr(SemContext *ctx, ASTNode *node);
static bool sem_if(SemContext *ctx, ASTNode *node);
static bool sem_else(SemContext *ctx, ASTNode *node);
static bool sem_while(SemContext *ctx, ASTNode *node);
static bool sem_return(SemContext *ctx, ASTNode *node);

/* helpers */
static SymInfo *sem_lookup_var(SemContext *ctx, const char *name);

/* ---------------------------
   Context management
   --------------------------- */

static SemContext *sem_ctx_create(void) {
    SemContext *ctx = calloc(1, sizeof(SemContext));
    if (!ctx) error_exit(99, "Out of memory (SemContext)\n");

    ctx->global_scope  = symtable_create(NULL);
    ctx->current_scope = ctx->global_scope;
    ctx->has_main_noargs = false;
    return ctx;
}

static void sem_ctx_free(SemContext *ctx) {
    if (!ctx) return;
    symtable_free(ctx->global_scope);
    free(ctx);
}

static void sem_enter_scope(SemContext *ctx) {
    SymTable *new_scope = symtable_create(ctx->current_scope);
    if (!new_scope) error_exit(99, "Out of memory (scope)\n");
    ctx->current_scope = new_scope;
}

static void sem_leave_scope(SemContext *ctx) {
    SymTable *parent = ctx->current_scope->next;
    symtable_free(ctx->current_scope);
    ctx->current_scope = parent;
}

/* ---------------------------
   Public driver
   --------------------------- */

bool sem_analyze(ASTNode *root)
{
    SemContext *ctx = sem_ctx_create();

    if (!sem_register_functions(ctx, root)) {
        sem_ctx_free(ctx);
        return false;
    }

    bool ok = sem_visit(ctx, root);

    if (!ctx->has_main_noargs) {
        error_exit(3, "Semantic error: missing main() with no parameters\n");
        ok = false;
    }

    sem_ctx_free(ctx);
    return ok;
}

/* ---------------------------
   Helpers
   --------------------------- */

static SymInfo *sem_lookup_var(SemContext *ctx, const char *name)
{
    SymInfo *s = symtable_find(ctx->current_scope, name);
    if (s && s->kind == SYM_VAR)
        return s;
    return NULL;
}

/* ---------------------------
   First pass: register functions
   --------------------------- */

static bool sem_register_functions(SemContext *ctx, ASTNode *node) {
    if (!node) return true;

    if (node->type == AST_FUNCTION_DEF) {
        if (!sem_function_def(ctx, node))
            return false;
    }

    for (int i = 0; i < node->child_count; ++i) {
        if (!sem_register_functions(ctx, node->children[i]))
            return false;
    }
    return true;
}

/* ---------------------------
   Dispatcher for second pass
   --------------------------- */

static bool sem_visit(SemContext *ctx, ASTNode *node) {
    if (!node) return true;

    switch (node->type) {
        case AST_FUNCTION_DEF: {
            ASTNode *name_node = node->children[0];
            ASTNode *kind_node = node->children[1];
            const char *name = name_node->token->lexeme;

            switch (kind_node->type) {
                case AST_FUNCTION:
                    return sem_normal_function(ctx, name, kind_node);
                case AST_GETTER:
                    return sem_getter(ctx, name, kind_node);
                case AST_SETTER:
                    return sem_setter(ctx, name, kind_node);
                default:
                    error_exit(99, "Internal error: bad function kind\n");
            }
        }

        case AST_BLOCK:
            return sem_block(ctx, node);

        case AST_VAR_DECL:
            return sem_var_decl(ctx, node);

        case AST_ASSIGN:
            return sem_assign(ctx, node);

        case AST_CALL:
            return sem_call(ctx, node);

        case AST_EXPR:
            return sem_expr(ctx, node);

        case AST_IF:
            return sem_if(ctx, node);

        case AST_ELSE:
            return sem_else(ctx, node);

        case AST_WHILE:
            return sem_while(ctx, node);

        case AST_RETURN:
            return sem_return(ctx, node);

        default:
            /* generic recursive traversal for other node types */
            for (int i = 0; i < node->child_count; ++i)
                if (!sem_visit(ctx, node->children[i]))
                    return false;
            return true;
    }
}

/* ---------------------------
   Function registration (pass 1)
   --------------------------- */

static bool sem_function_def(SemContext *ctx, ASTNode *def) {
    if (def->child_count != 2) {
        error_exit(99, "Internal error: bad FUNCTION_DEF arity\n");
    }

    ASTNode *name_node = def->children[0];
    ASTNode *kind_node = def->children[1];
    const char *name   = name_node->token->lexeme;

    if (!name || name_node->type != AST_IDENTIFIER) {
        error_exit(99, "Internal error: function name missing\n");
    }

    if (kind_node->type == AST_FUNCTION) {
        ASTNode *params = kind_node->children[0];   /* PARAM_LIST */
        int arity = params ? params->child_count : 0;

        char *key = make_func_key(name, arity);
        SymInfo *existing = symtable_find(ctx->global_scope, key);
        if (existing) {
            error_exit(4,
                       "Semantic error: redefinition of function %s with %d params\n",
                       name, arity);
        }

        SymInfo *sym = calloc(1, sizeof(SymInfo));
        if (!sym) error_exit(99, "Out of memory (SymInfo func)\n");

        sym->kind = SYM_FUNC;
        sym->info.func.arity = arity;
        sym->info.func.param_type_mask = NULL;
        sym->info.func.ret_type_mask   = TYPEMASK_ALL;
        sym->info.func.declared        = true;
        sym->info.func.defined         = true;
        sym->info.func.is_getter       = false;
        sym->info.func.is_setter       = false;

        if (!symtable_insert(ctx->global_scope, key, sym)) {
            error_exit(99, "Internal error: symtable_insert failed\n");
        }

        if (strcmp(name, "main") == 0 && arity == 0)
            ctx->has_main_noargs = true;

        free(key);
        return true;
    }
    else if (kind_node->type == AST_GETTER) {
        return sem_getter(ctx, name, kind_node);
    }
    else if (kind_node->type == AST_SETTER) {
        return sem_setter(ctx, name, kind_node);
    }

    error_exit(99, "Internal error: unknown function kind\n");
}

/* ---------------------------
   Function / getter / setter bodies (pass 2)
   --------------------------- */

static bool sem_getter(SemContext *ctx,
                       const char *name,
                       ASTNode *getter_node)
{
    /* use arity 0 key for getter */
    char *key = make_func_key(name, 0);
    SymInfo *existing = symtable_find(ctx->global_scope, key);

    if (existing) {
        if (existing->kind == SYM_FUNC &&
            existing->info.func.is_getter) {
            error_exit(4,
                       "Semantic error: duplicate static getter '%s'\n",
                       name);
        } else {
            error_exit(4,
                       "Semantic error: '%s' already used for a different symbol\n",
                       name);
        }
    }

    SymInfo *sym = calloc(1, sizeof(SymInfo));
    if (!sym) error_exit(99, "Out of memory (getter SymInfo)\n");

    sym->kind = SYM_FUNC;
    sym->info.func.arity = 0;
    sym->info.func.param_type_mask = NULL;
    sym->info.func.ret_type_mask   = TYPEMASK_ALL;
    sym->info.func.declared        = true;
    sym->info.func.defined         = true;
    sym->info.func.is_getter       = true;
    sym->info.func.is_setter       = false;

    if (!symtable_insert(ctx->global_scope, key, sym)) {
        error_exit(99, "Internal error: symtable_insert(getter) failed\n");
    }
    free(key);

    /* body: getter has children[0] = BLOCK */
    sem_enter_scope(ctx);
    ASTNode *body = getter_node->child_count > 0 ? getter_node->children[0] : NULL;
    if (body && !sem_visit(ctx, body)) {
        sem_leave_scope(ctx);
        return false;
    }
    sem_leave_scope(ctx);
    return true;
}

static bool sem_setter(SemContext *ctx,
                       const char *name,
                       ASTNode *setter_node)
{
    char *key = make_func_key(name, 1);
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

    SymInfo *sym = calloc(1, sizeof(SymInfo));
    if (!sym) error_exit(99, "Out of memory (setter SymInfo)\n");

    sym->kind = SYM_FUNC;
    sym->info.func.arity = 1;
    sym->info.func.param_type_mask = NULL;
    sym->info.func.ret_type_mask   = TYPEMASK_ALL;
    sym->info.func.declared        = true;
    sym->info.func.defined         = true;
    sym->info.func.is_getter       = false;
    sym->info.func.is_setter       = true;

    if (!symtable_insert(ctx->global_scope, key, sym)) {
        error_exit(99, "Internal error: symtable_insert(setter) failed\n");
    }
    free(key);

    /* setter: children[0] = param ID, children[1] = BLOCK */
    sem_enter_scope(ctx);

    ASTNode *param_node = setter_node->children[0];
    ASTNode *body       = setter_node->children[1];

    SymInfo *param_sym = calloc(1, sizeof(SymInfo));
    if (!param_sym) error_exit(99, "Out of memory (setter param)\n");

    param_sym->kind = SYM_VAR;
    param_sym->info.var.is_global = false;
    param_sym->info.var.type_mask = TYPEMASK_ALL;

    if (!symtable_insert(ctx->current_scope,
                         param_node->token->lexeme,
                         param_sym)) {
        error_exit(4,
                   "Semantic error: duplicate parameter '%s' in setter '%s'\n",
                   param_node->token->lexeme,
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
                                const char *name,
                                ASTNode *func_node)
{
    ASTNode *params = func_node->children[0];
    ASTNode *body   = func_node->children[1];

    sem_enter_scope(ctx);

    for (int i = 0; params && i < params->child_count; ++i) {
        ASTNode *param_id = params->children[i];

        SymInfo *var_sym = calloc(1, sizeof(SymInfo));
        if (!var_sym) error_exit(99, "Out of memory (func param)\n");

        var_sym->kind = SYM_VAR;
        var_sym->info.var.is_global = false;
        var_sym->info.var.type_mask = TYPEMASK_ALL;

        if (!symtable_insert(ctx->current_scope,
                             param_id->token->lexeme,
                             var_sym)) {
            error_exit(4,
                       "Semantic error: duplicate parameter '%s' in function '%s'\n",
                       param_id->token->lexeme,
                       name);
        }
    }

    bool ok = sem_visit(ctx, body);
    sem_leave_scope(ctx);
    return ok;
}

/* ---------------------------
   Blocks & statements
   --------------------------- */

static bool sem_block(SemContext *ctx, ASTNode *node)
{
    sem_enter_scope(ctx);

    for (int i = 0; i < node->child_count; i++) {
        ASTNode *stmt = node->children[i];

        /* disallow function defs inside blocks (class-level only) */
        if (stmt->type == AST_FUNCTION_DEF) {
            error_exit(4,
                       "Semantic error: function definitions only allowed at class scope\n");
        }

        if (!sem_visit(ctx, stmt))
            return false;
    }

    sem_leave_scope(ctx);
    return true;
}

static bool sem_var_decl(SemContext *ctx, ASTNode *node)
{
    const char *name = node->token->lexeme;
    bool is_gid = (node->token->type == TOK_GID);

    /* Rule 1: GIDs can only be declared in global scope */
    if (is_gid && ctx->current_scope != ctx->global_scope) {
        error_exit(4,
            "Semantic error: cannot declare global variable '%s' in local scope\n",
            name);
    }

    /* Rule 2: check duplicates only in *this* scope */
    SymInfo *existing = symtable_find(ctx->current_scope, name);
    if (existing) {
        error_exit(4,
            "Semantic error: duplicate variable '%s'\n",
            name);
    }

    /* Rule 3: declare the variable explicitly */
    SymInfo *sym = calloc(1, sizeof(SymInfo));
    if (!sym) error_exit(99, "Out of memory");

    sym->kind = SYM_VAR;
    sym->info.var.is_global = (ctx->current_scope == ctx->global_scope);
    sym->info.var.type_mask = TYPEMASK_ALL;

    if (!symtable_insert(ctx->current_scope, name, sym)) {
        error_exit(99, "Internal error: symtable_insert failed\n");
    }

    /* Rule 4: initializer if present */
    if (node->child_count == 1) {
        ASTNode *assign = node->children[0];
        ASTNode *expr = assign->children[0];
        return sem_visit(ctx, expr);
    }

    return true;
}

/* assignment from statement_sid: token = identifier, child[0] = expr */
static bool sem_assign(SemContext *ctx, ASTNode *node)
{
    const char *name = node->token ? node->token->lexeme : NULL;

    if (!name) {
        /* we only expect bare ASSIGN in var-decls, which we handled there */
        error_exit(99, "Internal error: unexpected anonymous ASSIGN\n");
    }

    SymInfo *sym = sem_lookup_var(ctx, name);

    if (sym) {
        /* normal variable assignment */
        if (node->child_count != 1) {
            error_exit(99, "Internal error: ASSIGN arity\n");
        }
        return sem_visit(ctx, node->children[0]);
    }

    /* maybe it's a setter? */
    char *key = make_func_key(name, 1);
    SymInfo *fsym = symtable_find(ctx->global_scope, key);
    free(key);

    if (fsym && fsym->kind == SYM_FUNC && fsym->info.func.is_setter) {
        if (node->child_count != 1) {
            error_exit(99, "Internal error: setter assignment arity\n");
        }
        /* semantically: treat as setter(name, expr) – just visit expr */
        return sem_visit(ctx, node->children[0]);
    }

    /* neither variable nor setter -> undefined or invalid target */
    error_exit(3, "Semantic error: assignment to undefined identifier '%s'\n", name);
    return false;
}

/* function call: token = identifier, children = arguments */
static bool sem_call(SemContext *ctx, ASTNode *node)
{
    const char *name = node->token->lexeme;
    int arity = node->child_count;

    char *key = make_func_key(name, arity);
    SymInfo *fsym = symtable_find(ctx->global_scope, key);
    free(key);

    if (!fsym) {
        error_exit(3,
                   "Semantic error: call to undefined function '%s' with %d args\n",
                   name, arity);
    }

    if (fsym->kind != SYM_FUNC) {
        error_exit(5,
                   "Semantic error: '%s' is not a function\n", name);
    }

    /* visit arguments */
    for (int i = 0; i < node->child_count; ++i) {
        if (!sem_visit(ctx, node->children[i]))
            return false;
    }

    return true;
}

/* expression: with current fake parse_expr, this is either
   - identifier / GID → must exist (var or getter or 0-arg func)
   - literal → nothing to check for semantics */
static bool sem_expr(SemContext *ctx, ASTNode *node)
{
    if (!node->token) return true;

    switch (node->token->type) {
        case TOK_IDENTIFIER:
        case TOK_GID: {
            const char *name = node->token->lexeme;

            /* 1) try variable in scope */
            SymInfo *vsym = sem_lookup_var(ctx, name);
            if (vsym) return true;

            /* 2) try getter / zero-arg function */
            char *key = make_func_key(name, 0);
            SymInfo *fsym = symtable_find(ctx->global_scope, key);
            free(key);

            if (fsym && fsym->kind == SYM_FUNC) {
                /* ok: getter or zero-arg func used as value */
                return true;
            }

            error_exit(3,
                       "Semantic error: use of undefined identifier '%s'\n",
                       name);
        }

        default:
            /* literals etc – nothing extra here */
            return true;
    }
}

static bool sem_if(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 2) {
        error_exit(99, "Internal error: IF arity\n");
    }

    ASTNode *cond = node->children[0];
    ASTNode *then_block = node->children[1];

    if (!sem_visit(ctx, cond))
        return false;
    if (!sem_visit(ctx, then_block))
        return false;

    return true;
}

static bool sem_else(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 1) {
        error_exit(99, "Internal error: ELSE arity\n");
    }
    return sem_visit(ctx, node->children[0]);
}

static bool sem_while(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 2) {
        error_exit(99, "Internal error: WHILE arity\n");
    }

    ASTNode *cond = node->children[0];
    ASTNode *body = node->children[1];

    if (!sem_visit(ctx, cond))
        return false;
    if (!sem_visit(ctx, body))
        return false;

    return true;
}

static bool sem_return(SemContext *ctx, ASTNode *node)
{
    /* optional expression child */
    if (node->child_count == 1) {
        return sem_visit(ctx, node->children[0]);
    }
    return true;
}
