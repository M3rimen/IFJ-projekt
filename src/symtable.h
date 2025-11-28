#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

#define TYPEMASK_NUM      0b0001
#define TYPEMASK_STRING   0b0010
#define TYPEMASK_NULL     0b0100
#define TYPEMASK_BOOL     0b1000    // if BOOL type is added in the language

#define TYPEMASK_ALL      0b1111

typedef unsigned char TypeMask;

/* -------------------- Types -------------------- */
typedef enum {
    SYM_VAR,
    SYM_FUNC
} SymKind;

typedef struct VarInfo {
    bool is_global;
    TypeMask type_mask;   // bitmask: T_NUM | T_STRING | T_NULL
} VarInfo;

typedef struct FuncInfo {
    int arity;
    TypeMask *param_type_mask; // array of type masks
    TypeMask ret_type_mask;  
    bool declared;
    bool defined;
    
    bool is_getter;
    bool is_setter;
    bool is_builtin;
} FuncInfo;

typedef struct SymInfo {
    SymKind kind;
    union {
        VarInfo var;
        FuncInfo func;
    } info;
} SymInfo;

typedef struct SymNode {
    char *key;
    SymInfo *sym;
    struct SymNode *left;
    struct SymNode *right;
} SymNode;

typedef struct SymTable {
    SymNode *root;
    struct SymTable *next;  
    struct SymTable *prev;
} SymTable;

/* -------------------- API -------------------- */

// create / destroy
SymTable *symtable_create(SymTable *parent);
void symtable_free(SymTable *table);

// insert & lookup
SymInfo *bst_find(SymNode *root, const char *key);
bool symtable_insert(SymTable *table, const char *key, SymInfo *sym);
SymInfo *symtable_find(SymTable *table, const char *key);

// key generator for overload
char *make_func_key(const char *name, int arity);

char *make_setter_key(const char *name);
char *make_getter_key(const char *name);

#endif