#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "err.h"


// Globálny aktuálny token
static Token current_token;

static void next_token() {
    current_token = scanner_next();
}

static const char *tok2symbol(TokenType t) {
    switch (t) {
        case TOK_IDENTIFIER: return "identifier";
        case TOK_GID:        return "global identifier";
        case TOK_KEYWORD:    return "keyword";
        case TOK_INT:        return "integer";
        case TOK_FLOAT:      return "float";
        case TOK_HEX:        return "hexadecimal";
        case TOK_STRING:     return "string";

        case TOK_PLUS:   return "+";
        case TOK_MINUS:  return "-";
        case TOK_STAR:   return "*";
        case TOK_SLASH:  return "/";
        case TOK_ASSIGN: return "=";
        case TOK_EQ:     return "==";
        case TOK_NE:     return "!=";
        case TOK_LT:     return "<";
        case TOK_LE:     return "<=";
        case TOK_GT:     return ">";
        case TOK_GE:     return ">=";

        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_COMMA:  return ",";
        case TOK_DOT:    return ".";
        case TOK_SEMICOLON: return ";";
        case TOK_COLON:  return ":";
        case TOK_QUESTION: return "?";

        case TOK_EOL: return "EOL";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        default: return "unknown";
    }
}



static void expect(TokenType type) {
    if (current_token.type != type) {
        error_exit(2,
        "Syntax error: expected '%s', got '%s' (lexeme: '%s')\n",
        tok2symbol(type),
        tok2symbol(current_token.type),
        current_token.lexeme ? current_token.lexeme : "<none>");
    }
    next_token();
}

static int is_keyword(const char *kw) {
    if (current_token.type != TOK_KEYWORD) return 0;
    if (!current_token.lexeme) return 0;
    return strcmp(current_token.lexeme, kw) == 0;
}

static void eat_eol_o(){
    while(current_token.type == TOK_EOL){
        next_token();
        eat_eol();
    }
}

static void eat_eol_m(void) {
    if (current_token.type != TOK_EOL) {
        error_exit(2, "Syntax error: expected end-of-line\n");
    }
    while (current_token.type == TOK_EOL) {
        next_token();
    }
}


void parser(){
    next_token();
    parse_prog();

    expect(TOK_EOF);
}

static void parser_prolog(){
    if (!is_keyword("import")){
        error_exit(2,"expected 'import' at the start of program \n");
    }
    next_token();

    eat_eol();

    if (current_token.type != TOK_STRING || !current_token.lexeme
        || strcmp(current_token.lexeme, "ifj25") != 0){
            error_exit(2 ,"expected 'import' at the start of program \n");
    }
    next_token();

    if (!is_keyword("for")){
        error_exit(2,"expected 'for' after import \n");
    }
    next_token();

    if (!is_keyword("Ifj")){
        error_exit(2,"expected 'Ifj' after for \n");
    }
    next_token();
    eat_eol_m();
    
}

static void parser_class_def(){
    if (!is_keyword("class")){
        error_exit(2 ,"expected 'class' at the start of class \n");
    }
    next_token();

    if (current_token.type != TOK_IDENTIFIER ||
        !current_token.lexeme ||
        strcmp(current_token.lexeme, "Program") != 0) {
        error_exit(2,"expected class name 'Program' after class\n");
    }
    next_token();
    expect(TOK_LBRACE);
    eat_eol_m();

    parser_function_defs();

    expect(TOK_RBRACE);
}

static void parser_function_defs(){
    while(is_keyword("static")){
        parser_function_def();
    }
}

static void parser_function_def(){
    if (!is_keyword("static")){
        error_exit(2,"expected 'static' at the start of function\n");
    }
    next_token();
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected function name after 'static'\n");
    }
    next_token();

    parser_function_kind();
}
static void parser_function_kind(void) {
    switch (current_token.type) {

        case TOK_LPAREN:
            parser_function_sig();
            break;

        case TOK_LBRACE:
            parser_getter_sig();
            break;

        case TOK_ASSIGN:
            parser_setter_sig();
            break;

        default:
            error_exit(2, "Syntax error: expected '(', '{' or '=' after function name");
    }
}

static void parser_function_sig(){
    expect(TOK_LPAREN);
    param_list();
    expect(TOK_RPAREN);
    block();
    eat_eol_m();
}

static void parser_getter_sig(){
    expect(TOK_LBRACE);
    block();
    eat_eol_m();
}

static void parser_setter_sig(){
    expect(TOK_ASSIGN);
    expect(TOK_LPAREN);
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected setter name after left parentace\n");
    }
    next_token();
    expect(TOK_RPAREN);
    block();
    eat_eol_m();
}

static void param_list(){
    if (current_token.type == TOK_RPAREN) {
    return;
    }   
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected setter id after 'static'\n");
    }
    next_token();
    if (current_token.type == TOK_COMMA){
    param_more();
    }
}
static void param_more(){
    expect(TOK_COMMA);
    eat_eol_o();
    param_list();

}

static void block(){
    expect(TOK_LBRACE);
    eat_eol_o();
    parser_statements();
    expect(TOK_RBRACE);
}

static void parser_statements() {

    while (
            is_keyword("var") ||
            is_keyword("return") ||
            is_keyword("if") ||
            is_keyword("while") || current_token.type == TOK_IDENTIFIER ||
            current_token.type == TOK_GID) {
        statement();
    }

}

typedef enum {
    KW_NONE = 0,
    KW_VAR,
    KW_RETURN,
    KW_IF,
    KW_WHILE
} KeywordKind;

static KeywordKind get_keyword(void) {
    if (current_token.type != TOK_KEYWORD)
        return KW_NONE;

    if (strcmp(current_token.lexeme, "var") == 0) return KW_VAR;
    if (strcmp(current_token.lexeme, "return") == 0) return KW_RETURN;
    if (strcmp(current_token.lexeme, "if") == 0) return KW_IF;
    if (strcmp(current_token.lexeme, "while") == 0) return KW_WHILE;

    return KW_NONE;
}

static void statement_var(void) {

    next_token(); 

    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2, "expected identifier after 'var'\n");
    }
    next_token();

    var_tail();
}

static void statement_return(void) {

    next_token();  

    return_tail();
}

static void statement_if(void) {

    next_token();               

    expect(TOK_LPAREN);
    parse_expr();               // precedence parser
    expect(TOK_RPAREN);

    block();

    if (!is_keyword("else")) {
        error_exit(2, "expected 'else' after if-block\n");
    }
    next_token();

    block();

    eat_eol_m();
}

static void statement_while(void) {

    next_token();               

    expect(TOK_LPAREN);
    parse_expr();
    expect(TOK_RPAREN);

    block();

    eat_eol_m();
}

static void statement_sid(void) {

    next_token();   

    id_tail();      
}

static void parser_statement() {

    
    if (current_token.type == TOK_IDENTIFIER ||
        current_token.type == TOK_GID) {

        statement_sid();
        return;
    }

    
    switch (get_keyword()) {

        case KW_VAR:
            statement_var();
            return;

        case KW_RETURN:
            statement_return();
            return;

        case KW_IF:
            statement_if();
            return;

        case KW_WHILE:
            statement_while();
            return;

        case KW_NONE:
        default:
            error_exit(2, "Syntax error: unexpected token at start of statement\n");
    }
}

static void var_tail(void) {

    if (current_token.type == TOK_ASSIGN) {
        next_token();               
        parse_expr();               // precedence parser
        eat_eol_m();                
        return;
    }

    // EOL_M
    eat_eol_m();
}

static void id_tail(void) {

    
    if (current_token.type == TOK_ASSIGN) {
        next_token();
        parse_expr();
        eat_eol_m();
        return;
    }

    
    if (current_token.type == TOK_LPAREN) {
        next_token();              
        arg_list();
        expect(TOK_RPAREN);
        eat_eol_m();
        return;
    }

    // EOLm
    eat_eol_m();
}

static void return_tail(void) {

    if (starts_expr(current_token)) {

        parse_expr();
        eat_eol_m();
        return;
    }

    eat_eol_m();
}

static void arg_list(void) {

    
    if (current_token.type == TOK_RPAREN) {
        return;
    }

    
    parse_expr();

    
    arg_more();
}

static void arg_more(){
    expect(TOK_COMMA);
    eat_eol_o();
    param_list();

}




