#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------
   Internal Helpers
   --------------------------------------------------------- */
static char *cstrdup(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p)
        memcpy(p, s, len);
    return p;
}

static SymNode *node_create(const char *key, SymInfo *sym) {
    SymNode *node = malloc(sizeof(SymNode));
    if (!node) return NULL;

    node->key = cstrdup(key);
    if (!node->key) {
        free(node);
        return NULL;
    }

    node->sym = sym;   // ownership transferred
    node->left = node->right = NULL;
    return node;
}

static void node_free(SymNode *node) {
    if (!node) return;

    // Free symbol-specific heap data
    if (node->sym) {
        if (node->sym->kind == SYM_FUNC) {
            // free array of param type_masks
            if (node->sym->info.func.param_type_mask)
                free(node->sym->info.func.param_type_mask);
        }
        free(node->sym);
    }

    free(node->key);
    free(node);
}

static void bst_free(SymNode *root) {
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    node_free(root);
}

/* Recursive insert (not scoped logic, only current table) */
static SymNode *bst_insert(SymNode *root, const char *key, SymInfo *sym, bool *inserted) {
    if (!root) {
        *inserted = true;
        return node_create(key, sym);
    }

    int cmp = strcmp(key, root->key);

    if (cmp == 0) {
        *inserted = false;
        return root;  // key already exists
    } 
    else if (cmp < 0) {
        root->left = bst_insert(root->left, key, sym, inserted);
    } 
    else {
        root->right = bst_insert(root->right, key, sym, inserted);
    }

    return root;
}

static SymInfo *bst_find(SymNode *root, const char *key) {
    if (!root) return NULL;
    int cmp = strcmp(key, root->key);

    if (cmp == 0)
        return root->sym;
    else if (cmp < 0)
        return bst_find(root->left, key);
    else
        return bst_find(root->right, key);
}


/* ---------------------------------------------------------
   Symbol Table API
   --------------------------------------------------------- */

SymTable *symtable_create(SymTable *parent) {
    SymTable *t = malloc(sizeof(SymTable));
    if (!t) return NULL;

    t->root = NULL;

    t->next = parent;  // parent scope
    t->prev = NULL;    // unused (or remove from struct entirely)

    return t;
}


void symtable_free(SymTable *table) {
    if (!table) return;
    bst_free(table->root);
    free(table);
}

bool symtable_insert(SymTable *table, const char *key, SymInfo *sym) {
    if (!table || !key || !sym) return false;

    bool inserted = false;
    table->root = bst_insert(table->root, key, sym, &inserted);
    return inserted;
}

/* Scoped lookup (current → parent → …) */
SymInfo *symtable_find(SymTable *table, const char *key) {
    for (SymTable *t = table; t != NULL; t = t->next) {
        SymInfo *s = bst_find(t->root, key);
        if (s) return s;
    }
    return NULL;
}

/* ---------------------------------------------------------
   Function Overload Key Generator
   name + "$" + arity
   Example:   make_func_key("add", 2) → "add$2"
   --------------------------------------------------------- */
char *make_func_key(const char *name, int arity) {
    if (!name) return NULL;

    char buf[32];
    snprintf(buf, sizeof(buf), "$%d", arity);

    size_t len_name = strlen(name);
    size_t len_suffix = strlen(buf);

    char *out = malloc(len_name + len_suffix + 1);
    if (!out) return NULL;

    memcpy(out, name, len_name);
    memcpy(out + len_name, buf, len_suffix + 1);

    return out;
}
