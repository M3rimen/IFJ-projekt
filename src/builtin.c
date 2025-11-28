#include "builtin.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ----------------------------------------------------
// Builtin definitions including type rules
// ----------------------------------------------------

static const BuiltinInfo builtin_table[] = {
    // name            arity  return-type         arg types...
    { "Ifj.readInt",     0,   TYPEMASK_NUM,       {0} },
    { "Ifj.readDouble",  0,   TYPEMASK_NUM,     {0} },
    { "Ifj.readString",  0,   TYPEMASK_STRING,    {0} },

    // Ifj.write is variadic, takes ANY type, returns null
    { "Ifj.write",      -1,   TYPEMASK_NULL,      { TYPEMASK_ALL } },

    // length(string) → int
    { "Ifj.length",      1,   TYPEMASK_NUM,
                          { TYPEMASK_STRING } },

    // substr(string, int, int) → string
    { "Ifj.substr",      3,   TYPEMASK_STRING,
                          { TYPEMASK_STRING, TYPEMASK_NUM, TYPEMASK_NUM } },

    // ord(string, int) → int
    { "Ifj.ord",         2,   TYPEMASK_NUM,
                          { TYPEMASK_STRING, TYPEMASK_NUM } },

    // chr(int) → string
    { "Ifj.chr",         1,   TYPEMASK_STRING,
                          { TYPEMASK_NUM } },
};

static const size_t builtin_count =
    sizeof(builtin_table) / sizeof(builtin_table[0]);


// ----------------------------------------------------
// Lookup functions
// ----------------------------------------------------

const BuiltinInfo *builtin_lookup(const char *name, int argc)
{
    for (size_t i = 0; i < builtin_count; ++i) {
        const BuiltinInfo *b = &builtin_table[i];

        if (strcmp(b->name, name) != 0)
            continue;

        if (b->arity < 0)    // variadic
            return b;
        if (b->arity == argc)
            return b;

        return NULL; // name ok, arity wrong
    }
    return NULL;
}

bool builtin_exists(const char *name)
{
    return builtin_lookup(name, -1) != NULL;
}

bool builtin_valid_arity(const char *name, int argc)
{
    return builtin_lookup(name, argc) != NULL;
}


// ----------------------------------------------------
// Builtin AST name extractor
// ----------------------------------------------------
//
// Input AST:
//
//  AST_FUNC_NAME
//      ├── AST_IDENTIFIER ("Ifj")
//      └── AST_IDENTIFIER ("readInt")
//
// Output malloc'ed string: "Ifj.readInt"
// ----------------------------------------------------

char *builtin_extract_name(ASTNode *funcname)
{
    if (!funcname || funcname->type != AST_FUNC_NAME ||
        funcname->child_count != 2)
        return NULL;

    ASTNode *ns = funcname->children[0];
    ASTNode *id = funcname->children[1];

    if (!ns->token || !id->token)
        return NULL;

    const char *s1 = ns->token->lexeme;
    const char *s2 = id->token->lexeme;

    size_t len = strlen(s1) + strlen(s2) + 2;
    char *out = malloc(len);
    if (!out) return NULL;

    snprintf(out, len, "%s.%s", s1, s2);
    return out;
}


// ----------------------------------------------------
// Codegen opcode mapping (optional)
// ----------------------------------------------------

const char *builtin_codegen_opcode(const char *name)
{
    if (!strcmp(name, "Ifj.readInt"))    return "READI";
    if (!strcmp(name, "Ifj.readDouble")) return "READF";
    if (!strcmp(name, "Ifj.readString")) return "READS";
    if (!strcmp(name, "Ifj.write"))      return "WRITE";
    if (!strcmp(name, "Ifj.length"))     return "STRLEN";
    if (!strcmp(name, "Ifj.substr"))     return "SUBSTR";
    if (!strcmp(name, "Ifj.ord"))        return "ORD";
    if (!strcmp(name, "Ifj.chr"))        return "CHR";

    return NULL;
}
