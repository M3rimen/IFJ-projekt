#ifndef PRECEDENC_H
#define PRECEDENC_H

#include <stdbool.h>
#include "token.h"

typedef enum {
    PREC_DOLLAR = 0,
    PREC_OPERAND,
    PREC_LPAR,
    PREC_RPAR,
    PREC_MUL,
    PREC_ADD,
    PREC_REL,
    PREC_IS,
    PREC_EQ,
    PREC_ERROR
} PrecTokenType;

typedef enum {
    UD = 0,   /* undefined (empty) */
    LW,       /* "<" lower – shift s markerom */
    EQ,       /* "=" equal – shift */
    HG        /* ">" higher – reduce */
} PrecAction;

typedef Token *(*GetTokenFn)(void *user_data);
typedef void   (*AdvanceTokenFn)(void *user_data);

/*
 * Hlavná funkcia PSA:
 *  - číta tokeny cez get_token/advance_token,
 *  - skončí pri prvom tokene, ktorý už do výrazu nepatrí,
 *  - vráti ERR_OK / ERR_SYNTAX / ERR_INTERNAL.
 *  - AST sa zatiaľ nevytvára, rieši sa iba precedenčná/syntaktická správnosť.
 */
int parse_expression_psa(GetTokenFn get_token,
                         AdvanceTokenFn advance_token,
                         void *user_data);

#endif
