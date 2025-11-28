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
    n->token = NULL;

    if (tok) {
        Token *copy = malloc(sizeof(Token));
        if (!copy)
            error_exit(99, "AST: malloc token failed\n");

        memcpy(copy, tok, sizeof(Token));

        if (tok->lexeme) {
            size_t len = strlen(tok->lexeme);
            copy->lexeme = malloc(len + 1);
            if (!copy->lexeme)
                error_exit(99, "AST: malloc lexeme failed\n");
            memcpy(copy->lexeme, tok->lexeme, len + 1);
        } else {
            copy->lexeme = NULL;
        }

        n->token = copy;
    }

    return n;
}

void ast_add_child(ASTNode *parent, ASTNode *child)
{
    ASTNode **new_arr = realloc(
        parent->children,
        sizeof(ASTNode*) * (parent->child_count + 1)
    );
    if (!new_arr)
        error_exit(99, "AST: realloc failed\n");

    parent->children = new_arr;
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void ast_free(ASTNode *n)
{
    if (!n) return;

    for (int i = 0; i < n->child_count; i++)
        ast_free(n->children[i]);

    free(n->children);

    if (n->token) {
        free(n->token->lexeme);
        free(n->token);
    }

    free(n);
}