#ifndef AST_H
#define AST_H

#include "token.h"
#include <stdbool.h>

typedef enum {
    AST_PROGRAM,
    AST_PROLOG,
    AST_CLASS,
    AST_FUNCTION_S,
    AST_FUNCTION_DEF,
    AST_FUNCTION_KIND,
    AST_FUNCTION,
    AST_GETTER,
    AST_SETTER,

    AST_PARAM_LIST,
    AST_ARG_LIST,      // ← pridané
    AST_BLOCK,
    AST_STATEMENTS,

    AST_VAR_DECL,
    AST_ASSIGN,
    AST_CALL,
    AST_RETURN,
    AST_IF,
    AST_ELSE,
    AST_WHILE,

    AST_EXPR,
    AST_IDENTIFIER,
    AST_GID,           // ← pridané
    AST_LITERAL,

    AST_STRING

} AST_TYPE;

typedef struct ASTNode {
    AST_TYPE type;
    Token *token;

    struct ASTNode **children;
    int child_count;

    unsigned char type_mask;     
    bool needs_dynamic_check;
} ASTNode;

ASTNode *ast_new(AST_TYPE type, Token *tok);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_free(ASTNode *node);

#endif