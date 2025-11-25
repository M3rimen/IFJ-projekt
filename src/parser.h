#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "token.h"

extern Token current_token;

// Spustenie parsera — vracia koreň AST stromu
ASTNode *parser_prog();

// Toto si nechávam externé, aby sa dali volať pri jednotkových testoch, 
// ale nie je povinné ich používať priamo.
ASTNode *parser_prolog();
ASTNode *parser_class_def();
ASTNode *parser_function_defs();
ASTNode *parser_function_def();
ASTNode *parser_function_kind();
ASTNode *parser_function_pick();
ASTNode *parser_getter_pick();
ASTNode *parser_setter_pick();

ASTNode *param_list();
void     param_more(ASTNode *list);

ASTNode *block();
void     parser_statements(ASTNode *block);
void     parser_statement(ASTNode *block);

ASTNode *statement_var();
ASTNode *statement_return();
ASTNode *statement_if();
ASTNode *statement_while();
ASTNode *statement_sid();

ASTNode *var_tail();
ASTNode *id_tail(Token *idtok);
ASTNode *return_tail();

void     arg_list(ASTNode *call);
void     arg_more(ASTNode *call);

ASTNode *parse_expr();   // placeholder, neskôr nahradíš precedenčnou analýzou

#endif // PARSER_H
