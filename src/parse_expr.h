#ifndef PSA_H
#define PSA_H

#include "token.h"
#include "ast.h"

typedef enum {
    GRP_MUL_DIV,    // *, /
    GRP_ADD_SUB,    // +, -
    GRP_REL,        // <, >, <=, >=
    GRP_IS,         // is
    GRP_EQ,         // ==, !=
    GRP_ID,         // identifikátory a literály
    GRP_LPAREN,     // (
    GRP_RPAREN,     // )
    GRP_EOF         // $
} PrecedenceGroup;

typedef enum {
    LT,
    GT,
    EQ,
    UD
} PrecedenceRelation;

typedef enum {
    PSA_OK,
    PSA_ERR_SYNTAX,
    PSA_ERR_INTERNAL
} PsaResult;

PrecedenceGroup token_to_group(const Token *tok);

PsaResult psa_parse_expression(Token first, Token *out_next, ASTNode **out_ast);

void parse_expression_or_die(Token first, Token *out_next, ASTNode **out_ast);

#endif
