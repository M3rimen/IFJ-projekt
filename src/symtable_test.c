#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "symtable_debug.h"

int main(void) {
    printf("=== SYMTABLE TEST HARNESS ===\n");

    SymTable *global = symtable_create(NULL);

    /* TEST 1: Insert variable */
    SymInfo *v = malloc(sizeof(SymInfo));
    v->kind = SYM_VAR;
    v->info.var.is_global = true;
    v->info.var.type_mask = TYPEMASK_NUM | TYPEMASK_NULL;

    symtable_insert(global, "x", v);

    /* TEST 2: Insert overloaded functions */
    char *k1 = make_func_key("foo", 1);
    char *k2 = make_func_key("foo", 2);

    SymInfo *f1 = malloc(sizeof(SymInfo));
    f1->kind = SYM_FUNC;
    f1->info.func.arity = 1;
    f1->info.func.ret_type_mask = TYPEMASK_STRING;
    f1->info.func.param_type_mask = malloc(sizeof(TypeMask));
    f1->info.func.param_type_mask[0] = TYPEMASK_NUM;
    f1->info.func.declared = true;
    f1->info.func.defined = false;

    SymInfo *f2 = malloc(sizeof(SymInfo));
    f2->kind = SYM_FUNC;
    f2->info.func.arity = 2;
    f2->info.func.ret_type_mask = TYPEMASK_NUM;
    f2->info.func.param_type_mask = malloc(sizeof(TypeMask) * 2);
    f2->info.func.param_type_mask[0] = TYPEMASK_STRING;
    f2->info.func.param_type_mask[1] = TYPEMASK_NUM;
    f2->info.func.declared = true;
    f2->info.func.defined = true;

    symtable_insert(global, k1, f1);
    symtable_insert(global, k2, f2);

    /* TEST 3: Lookup */
    printf("\nLookup x: ");
    SymInfo *lx = symtable_find(global, "x");
    print_syminfo(lx);

    printf("Lookup %s: ", k1);
    print_syminfo(symtable_find(global, k1));

    printf("Lookup %s: ", k2);
    print_syminfo(symtable_find(global, k2));
    printf("\n");

    /* PRINT ALL SCOPES */
    print_symtable(global);

    /* cleanup */
    free(k1);
    free(k2);
    symtable_free(global);

    return 0;
}
