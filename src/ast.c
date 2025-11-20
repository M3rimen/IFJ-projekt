
#include "ast.h"
#include "err.h"
#include "scanner.h"


ASTNode *nodeCreate(AST_TYPE type, Token *token) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {

        error_exit(99, "Memory allocation failed for AST node\n");
    }
    node->type = type;
    node->token = token;   // môže byť NULL pre neterminály
    node->left = NULL;
    node->right = NULL;

    
    return node;
}

void insertLeft(ASTNode *parent, ASTNode *leftChild) {
    if (!parent) return;
    parent->left = leftChild;
}

void insertRight(ASTNode *parent, ASTNode *rightChild) {
    if (!parent) return;
    parent->right = rightChild;
}
void freeAST(ASTNode *node) {
    if (!node) return;
    freeAST(node->left);
    freeAST(node->right);

    if (node->token) {
        free(node->token->lexeme);
        free(node->token);
    }
    free(node);
}

