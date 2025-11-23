#include "ast.h"
#include "err.h"
#include <stdlib.h>
#include <string.h>


ASTNode *ast_new(AST_TYPE type, Token *tok)
{
    ASTNode *n = malloc(sizeof(ASTNode));
    if (!n)
        error_exit(99, "AST: malloc failed\n");

    n->type = type;
    n->child_count = 0;
    n->children = NULL;

    if (tok) {
        Token *copy = malloc(sizeof(Token));
        if (!copy)
            error_exit(99, "AST: malloc token failed\n");

        memcpy(copy, tok, sizeof(Token));

        if (tok->lexeme)
            copy->lexeme = strdup(tok->lexeme);

        n->token = copy;
    } else {
        n->token = NULL;
    }

    return n;
}


void ast_add_child(ASTNode *parent, ASTNode *child)
{
    parent->children = realloc(
        parent->children,
        sizeof(ASTNode*) * (parent->child_count + 1)
    );

    if (!parent->children)
        error_exit(99, "AST: realloc failed\n");

    parent->children[parent->child_count] = child;
    parent->child_count++;
}


void ast_free(ASTNode *n)
{
    if (!n) return;

    for (size_t i = 0; i < n->child_count; i++)
        ast_free(n->children[i]);

    free(n->children);

    if (n->token) {
        free(n->token->lexeme);
        free(n->token);
    }

    free(n);
}
