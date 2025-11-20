#ifndef PSA_H
#define PSA_H

#include "token.h"

// -------------------- Precedence Groups Enum --------------------
typedef enum {
    GRP_MUL_DIV,    // *, /
    GRP_ADD_SUB,    // +, -`
    GRP_REL,        // <, >, <=, >=
    GRP_IS,         // is
    GRP_EQ,         // ==, !=
    GRP_ID,         // id
    GRP_LPAREN,     // (
    GRP_RPAREN,     // )
    GRP_EOF         // $
} PrecedenceGroup;


// -------------------- Precedence Relations --------------------
typedef enum {
    LT,    // <
    GT,    // >
    EQ,    // =
    UD     // undefined
} PrecedenceRelation;

// -------------------- Operator Precedence Table --------------------
extern PrecedenceRelation prec_table[9][9];

// -------------------- Token → Group --------------------
PrecedenceGroup token_to_group(const Token *tok);

// -------------------- PSA Parse Result --------------------
typedef enum {
    PSA_OK = 0,
    PSA_ERR_SYNTAX,
    PSA_ERR_INTERNAL
} PsaResult;

// Parse expression starting with already-read token `first`.
PsaResult psa_parse_expression(Token first, Token *out_next);

#endif // PSA_H
