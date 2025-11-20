#include "ast.h"
#include "err.h"
#include <stdlib.h>

ASTNode *ast_new(AST_TYPE type, Token *tok) {
    ASTNode *n = malloc(sizeof(ASTNode));
    if (!n) error_exit(99, "AST malloc failed");

    n->type = type;
    n->token = tok;
    n->children = NULL;
    n->child_count = 0;
    return n;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    parent->children = realloc(parent->children,
                               sizeof(ASTNode*) * (parent->child_count + 1));

    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void ast_free(ASTNode *node) {
    if (!node) return;

    for (int i = 0; i < node->child_count; i++)
        ast_free(node->children[i]);

    free(node->children);

    if (node->token) {
        free(node->token->lexeme);
        free(node->token);
    }

    free(node);
}
