// sem_analysis.c

#include "sem_analysis.h"
#include "symtable.h"
#include "ast.h"
#include "token.h"
#include "err.h"
#include "builtin.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ---------------------------------------------------------
   Global symtable (created in main.c)
   --------------------------------------------------------- */
extern SymTable *g_global_symtable;

/* ---------------------------------------------------------
   Forward declarations
   --------------------------------------------------------- */

static SemContext *sem_ctx_create(void);
static void        sem_ctx_free(SemContext *ctx);

static void        sem_enter_scope(SemContext *ctx);
static void        sem_leave_scope(SemContext *ctx);

static bool        sem_visit(SemContext *ctx, ASTNode *node);

static bool        sem_function_def(SemContext *ctx, ASTNode *def);
static bool        sem_normal_function(SemContext *ctx,
                                       const char *name,
                                       ASTNode *func_node);
static bool        sem_getter_body(SemContext *ctx,
                                   const char *name,
                                   ASTNode *getter_node);
static bool        sem_setter_body(SemContext *ctx,
                                   const char *name,
                                   ASTNode *setter_node);

static bool        sem_block(SemContext *ctx, ASTNode *block);
static bool        sem_var_decl(SemContext *ctx, ASTNode *node);
static bool        sem_assign(SemContext *ctx, ASTNode *node);
static bool        sem_call(SemContext *ctx, ASTNode *node);
static bool        sem_if(SemContext *ctx, ASTNode *node);
static bool        sem_else(SemContext *ctx, ASTNode *node);
static bool        sem_while(SemContext *ctx, ASTNode *node);
static bool        sem_return(SemContext *ctx, ASTNode *node);
static bool        sem_expr(SemContext *ctx, ASTNode *node);

/* helpers */
static SymInfo    *sem_lookup_var(SemContext *ctx, const char *name);
static SymInfo    *symtable_find_local(SymTable *table, const char *key);
static void        sem_register_func_record(SemContext *ctx, SymInfo *sym);

/* ---------------------------------------------------------
   Context
   --------------------------------------------------------- */

static SemContext *sem_ctx_create(void)
{
    SemContext *ctx = calloc(1, sizeof(SemContext));
    if (!ctx) error_exit(99, "Out of memory (SemContext)\n");

    if (!g_global_symtable)
        error_exit(99, "Global symtable not initialized\n");

    ctx->global_scope    = g_global_symtable;
    ctx->current_scope   = ctx->global_scope;
    ctx->has_main_noargs = false;
    ctx->func_list       = NULL;

    return ctx;
}

static void sem_ctx_free(SemContext *ctx)
{
    if (!ctx) return;

    /* Free all scopes created by sem_enter_scope,
       but NOT the global scope (freed in main). */
    SymTable *t = ctx->current_scope;
    while (t && t != ctx->global_scope) {
        SymTable *parent = t->next;
        symtable_free(t);
        t = parent;
    }

    // free func_list (SymInfo itself is owned by symtable)
    FuncRecord *fr = ctx->func_list;
    while (fr) {
        FuncRecord *n = fr->next;
        free(fr);
        fr = n;
    }

    free(ctx);
}

static void sem_enter_scope(SemContext *ctx)
{
    SymTable *new_scope = symtable_create(ctx->current_scope);
    if (!new_scope)
        error_exit(99, "Out of memory (scope)\n");
    ctx->current_scope = new_scope;
}

static void sem_leave_scope(SemContext *ctx)
{
    SymTable *old    = ctx->current_scope;
    SymTable *parent = old->next;
    symtable_free(old);
    ctx->current_scope = parent;
}

/* ---------------------------------------------------------
   Helpers
   --------------------------------------------------------- */

static SymInfo *symtable_find_local(SymTable *table, const char *key)
{
    if (!table) return NULL;
    return bst_find(table->root, key);
}

/* normal scoped lookup (local -> parent -> ...) */
static SymInfo *sem_lookup_var(SemContext *ctx, const char *name)
{
    SymInfo *s = symtable_find(ctx->current_scope, name);
    if (s && s->kind == SYM_VAR)
        return s;
    return NULL;
}

static void sem_register_func_record(SemContext *ctx, SymInfo *sym)
{
    if (!sym || sym->kind != SYM_FUNC) return;

    FuncRecord *rec = malloc(sizeof(FuncRecord));
    if (!rec) error_exit(99, "Out of memory (FuncRecord)\n");

    rec->sym  = sym;
    rec->next = ctx->func_list;
    ctx->func_list = rec;
}

/* ---------------------------------------------------------
   Public entry
   --------------------------------------------------------- */

bool sem_analyze(ASTNode *root)
{
    if (!root) return true;

    SemContext *ctx = sem_ctx_create();

    bool ok = sem_visit(ctx, root);

    /* Final check: all (user) functions that were declared must be defined.
       We do NOT put builtins into the symtable, so only user functions are here. */
    for (FuncRecord *fr = ctx->func_list; fr; fr = fr->next) {
        SymInfo *f = fr->sym;
        if (!f || f->kind != SYM_FUNC) continue;

        if (!f->info.func.defined) {
            error_exit(3,
                       "Semantic error: function declared but not defined\n");
            ok = false;
            break;
        }
    }

    /* Check main() with no params exists (and is defined) */
    if (!ctx->has_main_noargs) {
        error_exit(3, "Semantic error: missing main() with no parameters\n");
        ok = false;
    }

    sem_ctx_free(ctx);
    return ok;
}

/* ---------------------------------------------------------
   Dispatcher
   --------------------------------------------------------- */

static bool sem_visit(SemContext *ctx, ASTNode *node)
{
    if (!node) return true;

    switch (node->type) {
        case AST_PROGRAM:
        case AST_PROLOG:
        case AST_CLASS:
        case AST_FUNCTION_S:
        case AST_STATEMENTS:
            for (int i = 0; i < node->child_count; ++i)
                if (!sem_visit(ctx, node->children[i]))
                    return false;
            return true;

        case AST_FUNCTION_DEF:
            return sem_function_def(ctx, node);

        case AST_BLOCK:
            return sem_block(ctx, node);

        case AST_VAR_DECL:
            return sem_var_decl(ctx, node);

        case AST_ASSIGN:
            return sem_assign(ctx, node);

        case AST_CALL:
            return sem_call(ctx, node);

        case AST_IF:
            return sem_if(ctx, node);

        case AST_ELSE:
            return sem_else(ctx, node);

        case AST_WHILE:
            return sem_while(ctx, node);

        case AST_RETURN:
            return sem_return(ctx, node);

        case AST_EXPR:
        case AST_IDENTIFIER:
        case AST_GID:
        case AST_LITERAL:
            return sem_expr(ctx, node);

        default:
            // generic recursive for other node types (e.g., AST_FUNC_NAME)
            for (int i = 0; i < node->child_count; ++i)
                if (!sem_visit(ctx, node->children[i]))
                    return false;
            return true;
    }
}

/* ---------------------------------------------------------
   Function definitions
   --------------------------------------------------------- */

static bool sem_function_def(SemContext *ctx, ASTNode *def)
{
    if (def->child_count != 2)
        error_exit(99, "Internal: bad FUNCTION_DEF arity\n");

    ASTNode *name_node = def->children[0];
    ASTNode *kind_node = def->children[1];

    if (!name_node || !name_node->token || !name_node->token->lexeme)
        error_exit(99, "Internal: FUNCTION_DEF name missing\n");

    const char *name = name_node->token->lexeme;

    switch (kind_node->type) {
        case AST_FUNCTION:
            return sem_normal_function(ctx, name, kind_node);
        case AST_GETTER:
            return sem_getter_body(ctx, name, kind_node);
        case AST_SETTER:
            return sem_setter_body(ctx, name, kind_node);
        default:
            error_exit(99, "Internal: unknown function kind\n");
    }
    return false;
}

/* normal static function: children[0] = PARAM_LIST, children[1] = BLOCK */
static bool sem_normal_function(SemContext *ctx,
                                const char *name,
                                ASTNode *func_node)
{
    ASTNode *params = NULL;
    ASTNode *body   = NULL;

    if (func_node->child_count >= 1)
        params = func_node->children[0];
    if (func_node->child_count >= 2)
        body   = func_node->children[1];

    int arity = params ? params->child_count : 0;

    char *key = make_func_key(name, arity);
    if (!key) error_exit(99, "Out of memory (func key)\n");

    SymInfo *existing = symtable_find(ctx->global_scope, key);

    if (existing) {
        if (existing->kind != SYM_FUNC) {
            error_exit(3,
                       "Semantic error: '%s' used as both variable and function\n",
                       name);
        }
        if (existing->info.func.defined) {
            error_exit(4,
                       "Semantic error: redefinition of function '%s' with %d parameters\n",
                       name, arity);
        }
        existing->info.func.defined = true;
    } else {
        // in case parser did NOT insert function (e.g. no hybrid),
        // create symbol here
        SymInfo *sym = calloc(1, sizeof(SymInfo));
        if (!sym) error_exit(99, "Out of memory (SymInfo func)\n");

        sym->kind = SYM_FUNC;
        sym->info.func.arity           = arity;
        sym->info.func.param_type_mask = NULL;
        sym->info.func.ret_type_mask   = TYPEMASK_ALL;
        sym->info.func.declared        = true;
        sym->info.func.defined         = true;
        sym->info.func.is_getter       = false;
        sym->info.func.is_setter       = false;

        if (!symtable_insert(ctx->global_scope, key, sym))
            error_exit(99, "symtable_insert(function) failed\n");

        existing = sym;
    }

    sem_register_func_record(ctx, existing);

    if (strcmp(name, "main") == 0 && arity == 0 && existing->info.func.defined)
        ctx->has_main_noargs = true;

    free(key);

    // function body scope (params + body)
    sem_enter_scope(ctx);

    // insert params as locals
    if (params) {
        for (int i = 0; i < params->child_count; ++i) {
            ASTNode *p = params->children[i];
            const char *pname = p->token->lexeme;

            if (symtable_find_local(ctx->current_scope, pname)) {
                error_exit(4,
                           "Semantic error: duplicate parameter '%s' in function '%s'\n",
                           pname, name);
            }

            SymInfo *psym = calloc(1, sizeof(SymInfo));
            if (!psym) error_exit(99, "Out of memory (param)\n");

            psym->kind = SYM_VAR;
            psym->info.var.is_global = false;
            psym->info.var.type_mask = TYPEMASK_ALL;

            if (!symtable_insert(ctx->current_scope, pname, psym))
                error_exit(99, "symtable_insert(param) failed\n");
        }
    }

    // visit body (do NOT create extra scope if AST_BLOCK is a block node)
    if (body) {
        for (int i = 0; i < body->child_count; ++i)
            if (!sem_visit(ctx, body->children[i])) {
                sem_leave_scope(ctx);
                return false;
            }
    }

    sem_leave_scope(ctx);
    return true;
}

/* Getter: GETTER children[0] = BLOCK */
static bool sem_getter_body(SemContext *ctx,
                            const char *name,
                            ASTNode *getter_node)
{
    if (getter_node->child_count != 1)
        error_exit(99, "Internal: bad GETTER node\n");

    char *gkey = make_getter_key(name);
    if (!gkey) error_exit(99, "Out of memory (getter key)\n");

    SymInfo *sym = symtable_find(ctx->global_scope, gkey);
    if (!sym) {
        sym = calloc(1, sizeof(SymInfo));
        if (!sym) error_exit(99, "Out of memory (getter SymInfo)\n");
        sym->kind = SYM_FUNC;
        sym->info.func.arity           = 0;
        sym->info.func.param_type_mask = NULL;
        sym->info.func.ret_type_mask   = TYPEMASK_ALL;
        sym->info.func.declared        = true;
        sym->info.func.defined         = true;
        sym->info.func.is_getter       = true;
        sym->info.func.is_setter       = false;

        if (!symtable_insert(ctx->global_scope, gkey, sym))
            error_exit(99, "symtable_insert(getter) failed\n");
    }
    free(gkey);

    ASTNode *body = getter_node->children[0];

    sem_enter_scope(ctx);
    for (int i = 0; i < body->child_count; ++i) {
        if (!sem_visit(ctx, body->children[i])) {
            sem_leave_scope(ctx);
            return false;
        }
    }
    sem_leave_scope(ctx);
    return true;
}

/* Setter: SETTER children[0] = param identifier, children[1] = BLOCK */
static bool sem_setter_body(SemContext *ctx,
                            const char *name,
                            ASTNode *setter_node)
{
    if (setter_node->child_count != 2)
        error_exit(99, "Internal: bad SETTER node\n");

    ASTNode *param_node = setter_node->children[0];
    ASTNode *body       = setter_node->children[1];

    const char *pname = param_node->token->lexeme;

    char *skey = make_setter_key(name);
    if (!skey) error_exit(99, "Out of memory (setter key)\n");

    SymInfo *sym = symtable_find(ctx->global_scope, skey);
    if (!sym) {
        sym = calloc(1, sizeof(SymInfo));
        if (!sym) error_exit(99, "Out of memory (setter SymInfo)\n");
        sym->kind = SYM_FUNC;
        sym->info.func.arity           = 1;
        sym->info.func.param_type_mask = NULL;
        sym->info.func.ret_type_mask   = TYPEMASK_ALL;
        sym->info.func.declared        = true;
        sym->info.func.defined         = true;
        sym->info.func.is_getter       = false;
        sym->info.func.is_setter       = true;

        if (!symtable_insert(ctx->global_scope, skey, sym))
            error_exit(99, "symtable_insert(setter) failed\n");
    }
    free(skey);

    sem_enter_scope(ctx);

    // insert setter parameter as local variable
    SymInfo *psym = calloc(1, sizeof(SymInfo));
    if (!psym) error_exit(99, "Out of memory (setter param)\n");
    psym->kind = SYM_VAR;
    psym->info.var.is_global = false;
    psym->info.var.type_mask = TYPEMASK_ALL;

    if (!symtable_insert(ctx->current_scope, pname, psym))
        error_exit(4, "Semantic error: duplicate parameter '%s' in setter '%s'\n",
                   pname, name);

    for (int i = 0; i < body->child_count; ++i) {
        if (!sem_visit(ctx, body->children[i])) {
            sem_leave_scope(ctx);
            return false;
        }
    }

    sem_leave_scope(ctx);
    return true;
}

/* ---------------------------------------------------------
   Blocks & statements
   --------------------------------------------------------- */

static bool sem_block(SemContext *ctx, ASTNode *block)
{
    sem_enter_scope(ctx);

    for (int i = 0; i < block->child_count; ++i) {
        if (!sem_visit(ctx, block->children[i])) {
            sem_leave_scope(ctx);
            return false;
        }
    }

    sem_leave_scope(ctx);
    return true;
}

static bool sem_var_decl(SemContext *ctx, ASTNode *node)
{
    const char *name = node->token->lexeme;

    // disallow duplicate in same scope
    if (symtable_find_local(ctx->current_scope, name)) {
        error_exit(4,
                   "Semantic error: duplicate variable '%s' in same scope\n",
                   name);
    }

    SymInfo *sym = calloc(1, sizeof(SymInfo));
    if (!sym) error_exit(99, "Out of memory (var SymInfo)\n");

    sym->kind = SYM_VAR;
    sym->info.var.is_global = (ctx->current_scope == ctx->global_scope);
    sym->info.var.type_mask = TYPEMASK_ALL;

    if (!symtable_insert(ctx->current_scope, name, sym))
        error_exit(99, "symtable_insert(var) failed\n");

    // optional initializer: child[0] = AST_ASSIGN with child[0] = expr
    if (node->child_count == 1) {
        ASTNode *assign = node->children[0];
        ASTNode *expr   = assign->children[0];
        return sem_visit(ctx, expr);
    }

    return true;
}

/* left side assign:
 *  - TOK_GID => global variable (implicit create allowed)
 *  - TOK_IDENTIFIER => local/global var OR setter
 */
static bool sem_assign(SemContext *ctx, ASTNode *node)
{
    if (!node->token || node->child_count != 1)
        error_exit(99, "Internal: bad ASSIGN node\n");

    const char *name = node->token->lexeme;
    ASTNode *expr = node->children[0];

    if (node->token->type == TOK_GID) {
        // global variable
        SymInfo *g = symtable_find(ctx->global_scope, name);
        if (!g) {
            // implicit create
            g = calloc(1, sizeof(SymInfo));
            if (!g) error_exit(99, "Out of memory (implicit GID)\n");
            g->kind = SYM_VAR;
            g->info.var.is_global = true;
            g->info.var.type_mask = TYPEMASK_ALL;
            if (!symtable_insert(ctx->global_scope, name, g))
                error_exit(99, "symtable_insert(GID) failed\n");
        } else if (g->kind != SYM_VAR) {
            error_exit(3, "Semantic error: '%s' is not a variable\n", name);
        }

        return sem_visit(ctx, expr);
    }

    // identifier: try variable first
    SymInfo *v = sem_lookup_var(ctx, name);
    if (v) {
        if (v->kind != SYM_VAR) {
            error_exit(3, "Semantic error: '%s' is not a variable\n", name);
        }
        return sem_visit(ctx, expr);
    }

    // try setter
    char *skey = make_setter_key(name);
    if (!skey) error_exit(99, "Out of memory (setter key in assign)\n");

    SymInfo *setter = symtable_find(ctx->global_scope, skey);
    free(skey);

    if (setter && setter->kind == SYM_FUNC) {
        // semantically as setter(name, expr); just check expr
        return sem_visit(ctx, expr);
    }

    // undefined identifier on left side => create implicit global var
    SymInfo *g = calloc(1, sizeof(SymInfo));
    if (!g) error_exit(99, "Out of memory (implicit global assign)\n");
    g->kind = SYM_VAR;
    g->info.var.is_global = true;
    g->info.var.type_mask = TYPEMASK_ALL;
    if (!symtable_insert(ctx->global_scope, name, g))
        error_exit(99, "symtable_insert(implicit global) failed\n");

    return sem_visit(ctx, expr);
}

/* ---------------------------------------------------------
   Calls (normal + builtins Ifj.xxx)
   --------------------------------------------------------- */

static bool sem_call(SemContext *ctx, ASTNode *node)
{
    const char *name = NULL;
    bool        name_allocated = false;
    int         argc = 0;
    int         first_arg_index = 0;

    if (node->token && node->token->lexeme) {
        // Normal call from id_tail: CALL(token=name, children=args)
        name = node->token->lexeme;
        argc = node->child_count;
        first_arg_index = 0;
    } else if (node->child_count > 0 &&
               node->children[0]->type == AST_FUNC_NAME) {
        // Builtin-style or Ifj.xxx: first child is FUNC_NAME, rest are args
        char *full = builtin_extract_name(node->children[0]);
        if (!full)
            error_exit(99, "Internal: bad builtin FUNC_NAME\n");
        name = full;
        name_allocated = true;
        argc = node->child_count - 1;
        first_arg_index = 1;
    } else {
        error_exit(99, "Internal: CALL without name\n");
    }

    // check all argument expressions
    for (int i = first_arg_index; i < node->child_count; ++i) {
        if (!sem_visit(ctx, node->children[i])) {
            if (name_allocated) free((char *)name);
            return false;
        }
    }

    // try builtin
    const BuiltinInfo *b = builtin_lookup(name, argc);
    if (b) {
        // you could set node->type_mask = b->ret_type here if you want
        if (name_allocated) free((char *)name);
        return true;
    }

    // normal static function in Program class
    char *key = make_func_key(name, argc);
    if (!key) {
        if (name_allocated) free((char *)name);
        error_exit(99, "Out of memory (func key in call)\n");
    }

    SymInfo *f = symtable_find(ctx->global_scope, key);

    if (!f) {
        // create lazy forward declaration
        f = calloc(1, sizeof(SymInfo));
        if (!f) {
            free(key);
            if (name_allocated) free((char *)name);
            error_exit(99, "Out of memory (lazy func)\n");
        }

        f->kind = SYM_FUNC;
        f->info.func.arity           = argc;
        f->info.func.param_type_mask = NULL;
        f->info.func.ret_type_mask   = TYPEMASK_ALL;
        f->info.func.declared        = true;
        f->info.func.defined         = false;
        f->info.func.is_getter       = false;
        f->info.func.is_setter       = false;

        if (!symtable_insert(ctx->global_scope, key, f)) {
            free(key);
            if (name_allocated) free((char *)name);
            error_exit(99, "symtable_insert(lazy func) failed\n");
        }

        sem_register_func_record(ctx, f);
    } else if (f->kind != SYM_FUNC) {
        free(key);
        if (name_allocated) free((char *)name);
        error_exit(3,
                   "Semantic error: '%s' is not a function\n",
                   name);
    }

    free(key);
    if (name_allocated) free((char *)name);
    return true;
}

/* ---------------------------------------------------------
   Control flow
   --------------------------------------------------------- */

static bool sem_if(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 2)
        error_exit(99, "Internal: bad IF node\n");

    ASTNode *cond = node->children[0];
    ASTNode *then_block = node->children[1];

    if (!sem_visit(ctx, cond))
        return false;

    return sem_block(ctx, then_block);
}

static bool sem_else(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 1)
        error_exit(99, "Internal: bad ELSE node\n");

    return sem_block(ctx, node->children[0]);
}

static bool sem_while(SemContext *ctx, ASTNode *node)
{
    if (node->child_count != 2)
        error_exit(99, "Internal: bad WHILE node\n");

    ASTNode *cond = node->children[0];
    ASTNode *body = node->children[1];

    if (!sem_visit(ctx, cond))
        return false;

    return sem_block(ctx, body);
}

/* ---------------------------------------------------------
   Return
   --------------------------------------------------------- */

static bool sem_return(SemContext *ctx, ASTNode *node)
{
    (void)ctx;

    if (node->child_count == 0)
        return true;

    if (node->child_count == 1)
        return sem_visit(ctx, node->children[0]);

    error_exit(99, "Internal: bad RETURN node\n");
    return false;
}

/* ---------------------------------------------------------
   Expressions
   --------------------------------------------------------- */

static bool sem_expr(SemContext *ctx, ASTNode *node)
{
    // If expr is just a wrapper, dive into child
    if (node->type == AST_EXPR && node->child_count == 1 && !node->token) {
        return sem_expr(ctx, node->children[0]);
    }

    Token *tok = node->token;
    if (!tok) {
        // composite expression (once PSA builds real AST)
        for (int i = 0; i < node->child_count; ++i)
            if (!sem_expr(ctx, node->children[i]))
                return false;
        return true;
    }

    const char *name = tok->lexeme;

    switch (tok->type) {
        case TOK_IDENTIFIER: {
            if (!name) error_exit(99, "Internal: identifier name NULL\n");

            // variable?
            if (sem_lookup_var(ctx, name))
                return true;

            // global variable (non-GID) not declared -> allowed, implicit null
            // we don't insert symbol here; codegen can treat unknown as null.
            return true;
        }

        case TOK_GID: {
            // global var starting with "__"
            SymInfo *g = symtable_find(ctx->global_scope, name);
            if (!g) {
                g = calloc(1, sizeof(SymInfo));
                if (!g) error_exit(99, "Out of memory (implicit global read)\n");
                g->kind = SYM_VAR;
                g->info.var.is_global = true;
                g->info.var.type_mask = TYPEMASK_NULL;  // starts as null
                if (!symtable_insert(ctx->global_scope, name, g))
                    error_exit(99, "symtable_insert(implicit GID) failed\n");
            } else if (g->kind != SYM_VAR) {
                error_exit(3, "Semantic error: '%s' is not a variable\n", name);
            }
            return true;
        }

        case TOK_INT:
        case TOK_FLOAT:
        case TOK_HEX:
        case TOK_STRING:
            // literals always ok; you can set node->type_mask here if you want
            return true;

        default:
            // operators etc. handled structurally by PSA later
            // here we just accept and rely on runtime checks
            return true;
    }
}
