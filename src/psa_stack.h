#ifndef PSA_STACK_H
#define PSA_STACK_H

#include "token.h"
#include "parse_expr.h"
#include "ast.h"

typedef enum {
    SYM_TERMINAL,
    SYM_NONTERM,
    SYM_MARKER
} StackSymKind;

typedef enum {
    TYPE_NONE,
    TYPE_NUM,
    TYPE_STRING,
    TYPE_NULL,
    TYPE_BOOL
} ExprType;

typedef struct {
    StackSymKind     kind;
    TokenType        tok_type;
    PrecedenceGroup  group;
    ExprType         expr_type;
    ASTNode         *node;
} StackItem;

void stack_init(void);
void stack_clear(void);

void stack_push_terminal(const Token *tok, ASTNode *node);
void stack_push_nonterm(ExprType type, ASTNode *node);
void stack_push_marker(void);

StackItem  stack_pop(void);
StackItem *stack_top(void);
StackItem *stack_top_terminal(void);

void stack_insert_marker_after_top_terminal(void);

int stack_size(void);
int stack_is_eof_with_E_on_top(void);

#endif
