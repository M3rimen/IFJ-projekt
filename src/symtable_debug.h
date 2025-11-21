#ifndef SYMTABLE_DEBUG_H
#define SYMTABLE_DEBUG_H

#include "symtable.h"

/* Pretty-printers */

void print_typemask(TypeMask m);
void print_syminfo(const SymInfo *sym);
void print_bst(const SymNode *node, int depth);
void print_symtable(const SymTable *table);

#endif
