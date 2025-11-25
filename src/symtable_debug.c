#include <stdio.h>
#include "symtable_debug.h"

void print_typemask(TypeMask m) {
    printf("[");
    if (m & TYPEMASK_NUM) printf(" NUM ");
    if (m & TYPEMASK_STRING) printf(" STRING ");
    if (m & TYPEMASK_NULL) printf(" NULL ");
    printf("]");
}

void print_syminfo(const SymInfo *sym) {
    if (!sym) {
        printf("(null syminfo)\n");
        return;
    }

    if (sym->kind == SYM_VAR) {
        printf("VAR: global=%d, types=", sym->info.var.is_global);
        print_typemask(sym->info.var.type_mask);
        printf("\n");
    } else if (sym->kind == SYM_FUNC) {
        printf("FUNC: arity=%d, declared=%d, defined=%d\n",
               sym->info.func.arity,
               sym->info.func.declared,
               sym->info.func.defined);

        printf("   return types=");
        print_typemask(sym->info.func.ret_type_mask);
        printf("\n");

        printf("   param types:");
        for (int i = 0; i < sym->info.func.arity; i++) {
            printf(" ");
            print_typemask(sym->info.func.param_type_mask[i]);
        }
        printf("\n");
    }
}

void print_bst(const SymNode *node, int depth) {
    if (!node) return;

    print_bst(node->right, depth + 1);

    for (int i = 0; i < depth; i++)
        printf("    ");

    printf("[%s] ", node->key);
    print_syminfo(node->sym);

    print_bst(node->left, depth + 1);
}

void print_symtable(const SymTable *table) {
    int scope = 0;
    for (const SymTable *t = table; t; t = t->next) {
        printf("=== SCOPE %d ===\n", scope++);
        print_bst(t->root, 0);
        printf("\n");
    }
}
