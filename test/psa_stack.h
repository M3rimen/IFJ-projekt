#ifndef PSA_STACK_H
#define PSA_STACK_H

#include "token.h"
#include "psa.h"

// -------------------- PSA Stack Symbol Types --------------------
typedef enum {
    SYM_TERMINAL,
    SYM_NONTERM,
    SYM_MARKER
} StackSymKind;

// semantický typ výrazu
typedef enum {
    TYPE_NONE,
    TYPE_NUM,
    TYPE_STRING,
    TYPE_NULL,
    TYPE_BOOL
} ExprType;

// -------------------- PSA Stack Item --------------------
typedef struct {
    StackSymKind kind;   // TERMINAL / NONTERM / MARKER

    TokenType tok_type;  // iba ak TERMINAL
    PrecedenceGroup group;   // iba ak TERMINAL
    ExprType expr_type;  // iba ak NONTERM

} StackItem;

// -------------------- Stack API --------------------
void stack_init();
void stack_clear();
void stack_push_terminal(const Token *tok);
void stack_push_nonterm(ExprType type);
void stack_push_marker(); // push '<'

StackItem stack_pop();
StackItem* stack_top();
StackItem* stack_top_terminal();

int stack_size();

// Utility pre PSA
int stack_is_eof_with_E_on_top(); 
void stack_insert_marker_after_top_terminal(); 

#endif
