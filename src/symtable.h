#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

#define TYPEMASK_NUM      0b001
#define TYPEMASK_STRING   0b010
#define TYPEMASK_NULL     0b100

#define TYPEMASK_ALL      0b111

typedef unsigned char TypeMask;

/* -------------------- Types -------------------- */
typedef enum {
    SYM_VAR,
    SYM_FUNC
} SymKind;

typedef enum {
    TYPE_UNKNOWN,
    TYPE_NUM,
    TYPE_STRING,
    TYPE_NULL
} ValueType;


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
bool symtable_insert(SymTable *table, const char *key, SymInfo *sym);
SymInfo *symtable_find(SymTable *table, const char *key);

// key generator for overload
char *make_func_key(const char *name, int arity);

#endif