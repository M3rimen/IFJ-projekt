#ifndef AST_H
#define AST_H

#include <stdbool.h>  
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

typedef enum {
    // Non-terminal nodes 
    N_PROG,          // prog
    N_PROLOG,        // prolog
    N_CLASS_DEF,     // class_def
    N_FUNCTION_DEFS, // function_defs
    N_FUNCTION_DEF,  // function_def
    N_FUNCTION_KIND, // function_kind

    N_FUNCTION_SIG,  // function_sig
    N_GETTER_SIG,    // getter_sig
    N_SETTER_SIG,    // setter_sig

    N_PARAM_LIST,    // param_list
    N_PARAM_MORE,    // param_more

    N_BLOCK,         // block
    N_STATEMENTS,    // statements
    N_STATEMENT,     // statement

    N_VAR_TAIL,      // var_tail
    N_ID_TAIL,       // id_tail
    N_RETURN_TAIL,   // return_tail

    N_ARG_LIST,      // arg_list
    N_ARG_MORE,      // arg_more

    N_EXPR,          // expr 

    N_SID,           // SID ::= ID | GID
    N_FUNC_NAME,     // func_name ::= ID | Ifj . ID

    N_EOL_M,         // EOL_M
    N_EOL_O,         // EOL_O

    // Terminal nodes 
    T_ID,            // ID
    T_GID,           // GID (global identifier: __xxx)
    T_NUMBER,        // číselný literál (Num) - pre expr
    T_STRING,        // STRING literál

    T_STATIC,        // 'static'
    T_CLASS,         // 'class'
    T_IMPORT,        // 'import'
    T_FOR,           // 'for'
    T_IFJ,           // 'Ifj'
    T_PROGRAM,       // 'Program'

    T_VAR,           // 'var'
    T_RETURN,        // 'return'
    T_IF,            // 'if'
    T_ELSE,          // 'else'
    T_WHILE,         // 'while'

    T_ASSIGN,        // '='
    T_LPAREN,        // '('
    T_RPAREN,        // ')'
    T_LBRACE,        // '{'
    T_RBRACE,        // '}'
    T_COMMA,         // ','
    T_DOT,           // '.'

    // operátory pre precedence parser
    T_PLUS,          // '+'
    T_MINUS,         // '-'
    T_MULT,          // '*'
    T_DIV,           // '/'

    T_EOL,           // EOL (newline)
    T_EOF            // EOF
} AST_TYPE;


// AST node structure
typedef struct ASTNode {
    AST_TYPE type;
    struct ASTNode *right;
    struct ASTNode *left;
    Token *token;
} ASTNode;


ASTNode *nodeCreate(AST_TYPE type, Token *token);
void insertRight(ASTNode *parent, ASTNode *rightChild);
void insertLeft(ASTNode *parent, ASTNode *leftChild);
void freeAST(ASTNode *node);

#endif // AST_H
