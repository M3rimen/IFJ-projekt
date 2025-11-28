#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>
#include <stddef.h>
#include "ast.h"
#include "symtable.h"

// Any builtin can specify a type mask for each argument.
// For variadic builtins, the 'arg_types' apply to each arg.
typedef struct {
    const char *name;            // e.g., "Ifj.readInt"
    int  arity;                  // >=0 = fixed,  -1 = variadic
    unsigned ret_type;           // TYPEMASK_*
    unsigned arg_types[4];       // Up to 4 args (IFJ doesn't have more)
} BuiltinInfo;

// Lookup by combined name + arity
const BuiltinInfo *builtin_lookup(const char *name, int argc);

// Check only name
bool builtin_exists(const char *name);

// Check name + arity
bool builtin_valid_arity(const char *name, int argc);

// Extract "Ifj.xxx" from AST_FUNC_NAME (returns malloc'ed string)
char *builtin_extract_name(ASTNode *funcname_node);

// Optional: code generator opcode mapping
const char *builtin_codegen_opcode(const char *name);

#endif
