#include <stdio.h>
#include <stdlib.h>
#include "psa_stack.h"

static StackItem stack[256];
static int sp = -1;

void stack_init(void)
{
    sp = -1;
}

void stack_clear(void)
{
    sp = -1;
}

void stack_push_terminal(const Token *tok, ASTNode *node)
{
    if (sp >= 255) {
        fprintf(stderr, "PSA stack overflow\n");
        exit(1);
    }
    sp++;

    stack[sp].kind      = SYM_TERMINAL;
    stack[sp].tok_type  = tok->type;
    stack[sp].group     = token_to_group(tok);
    stack[sp].expr_type = TYPE_NONE;
    stack[sp].node      = node;
}

void stack_push_nonterm(ExprType type, ASTNode *node)
{
    if (sp >= 255) {
        fprintf(stderr, "PSA stack overflow\n");
        exit(1);
    }
    sp++;

    stack[sp].kind      = SYM_NONTERM;
    stack[sp].tok_type  = TOK_ERROR;  
    stack[sp].group     = GRP_EOF;
    stack[sp].expr_type = type;
    stack[sp].node      = node;
}

void stack_push_marker(void)
{
    if (sp >= 255) {
        fprintf(stderr, "PSA stack overflow\n");
        exit(1);
    }
    sp++;

    stack[sp].kind      = SYM_MARKER;
    stack[sp].tok_type  = TOK_ERROR;
    stack[sp].group     = GRP_EOF;
    stack[sp].expr_type = TYPE_NONE;
    stack[sp].node      = NULL;
}

StackItem stack_pop(void)
{
    if (sp < 0) {
        fprintf(stderr, "PSA stack underflow\n");
        exit(1);
    }
    return stack[sp--];
}

StackItem *stack_top(void)
{
    if (sp < 0) return NULL;
    return &stack[sp];
}

StackItem *stack_top_terminal(void)
{
    for (int i = sp; i >= 0; --i) {
        if (stack[i].kind == SYM_TERMINAL)
            return &stack[i];
    }
    return NULL;
}

void stack_insert_marker_after_top_terminal(void)
{
    if (sp >= 255) {
        fprintf(stderr, "PSA stack overflow (marker)\n");
        exit(1);
    }

    int idx = -1;
    for (int i = sp; i >= 0; --i) {
        if (stack[i].kind == SYM_TERMINAL) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        fprintf(stderr, "PSA stack: no terminal to insert marker after\n");
        exit(1);
    }

    for (int i = sp; i > idx; --i)
        stack[i + 1] = stack[i];

    sp++;

    stack[idx + 1].kind      = SYM_MARKER;
    stack[idx + 1].tok_type  = TOK_ERROR;
    stack[idx + 1].group     = GRP_EOF;
    stack[idx + 1].expr_type = TYPE_NONE;
    stack[idx + 1].node      = NULL;
}

int stack_size(void)
{
    return sp + 1;
}

int stack_is_eof_with_E_on_top(void)
{
    if (sp != 1) return 0;

    if (stack[0].kind == SYM_TERMINAL &&
        stack[0].tok_type == TOK_EOF &&
        stack[1].kind == SYM_NONTERM)
        return 1;

    return 0;
}
