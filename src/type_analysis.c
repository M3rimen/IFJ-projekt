#include "type_analysis.h"
#include <stdio.h>
#include <stdlib.h>

static TypeMask infer_expr(TypeContext *ctx, ASTNode *expr);
static TypeMask merge_mask(TypeMask a, TypeMask b);
static bool is_int_mask(TypeMask m);

// ---------------------------
// Entry
// ---------------------------
bool type_analyze(ASTNode *root, SymTable *global) {
    TypeContext ctx = {
        .global_scope = global,
        .current_scope = global
    };

    // Traverse AST and infer types
    // Only expressions and call sites matter; declarations do not
    // You can choose either one big traversal or per-node recursion

    // Basic generic traversal:
    switch (root->type) {
        default:
            for (int i = 0; i < root->child_count; i++) {
                if (!type_analyze(root->children[i], global))
                    return false;
            }
            break;
    }

    return true;
}

// ---------------------------
// Expression type inference
// ---------------------------
static TypeMask infer_expr(TypeContext *ctx, ASTNode *expr) {

    // TODO:
    // 1. If literal → return NUM, STRING, or NULL mask
    // 2. If identifier → get VarInfo.type_mask or getter return type
    // 3. If binary operator:
    //      - infer children recursively
    //      - check operator rules (static error 6 if impossible)
    // 4. If function call:
    //      - lookup function symbol
    //      - merge param_type_mask with given arguments
    //      - return function.ret_type_mask
    // 5. If runtime unknown types → return TYPEMASK_ALL

    return TYPEMASK_ALL;
}

// ---------------------------
// Helpers
// ---------------------------

static TypeMask merge_mask(TypeMask a, TypeMask b) {
    return a | b;
}

static bool is_int_mask(TypeMask m) {
    // For STATICAN: track Num vs. integer-Num
    // Basic project → treat all Num as non-integer unless literal
    return false;
}
