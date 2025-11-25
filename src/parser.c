#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "err.h"
#include "ast.h"

#include <string.h>


// Globálny aktuálny token
static Token current_token;

// ------------------------------
// Prototypy
// ------------------------------

static int starts_expr(Token t);


ASTNode *parser_prog();
ASTNode *parser_prolog();
ASTNode *parser_class_def();
ASTNode *parser_function_defs();
ASTNode *parser_function_def();
ASTNode *parser_function_kind();
ASTNode *parser_function_pick();
ASTNode *parser_getter_pick();
ASTNode *parser_setter_pick();

ASTNode *param_list();
void param_more(ASTNode *list);

ASTNode *block();
void parser_statements(ASTNode *blok);
void parser_statement(ASTNode *blok);

ASTNode *statement_var();
ASTNode *statement_return();
ASTNode *statement_if();
ASTNode *statement_while();
ASTNode *statement_sid();

ASTNode *var_tail();
ASTNode *id_tail();
ASTNode *return_tail();

void arg_list(ASTNode *call);
void arg_more(ASTNode *alist);

ASTNode *parse_expr();   // placeholder

// helpers
static void next_token();
static void eat_eol_o();
static void eat_eol_m();
static int is_keyword(const char *kw);
static const char *tok2symbol(TokenType t);

Token *copy_token(const Token *src);



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
    if (type == TOK_EOF) {
        next_token();
        return;
    }
    next_token();
}

static int is_keyword(const char *kw) {
    if (current_token.type != TOK_KEYWORD) return 0;
    if (!current_token.lexeme) return 0;
    return strcmp(current_token.lexeme, kw) == 0;
}

Token *copy_token(const Token *src)
{
    if (!src) return NULL;

    Token *t = malloc(sizeof(Token));
    if (!t)
        error_exit(99, "AST: malloc failed in copy_token()\n");

    t->type = src->type;

    if (src->lexeme) {
        size_t len = strlen(src->lexeme);
        t->lexeme = malloc(len + 1);
        if (!t->lexeme)
            error_exit(99, "AST: malloc failed for token lexeme\n");
        memcpy(t->lexeme, src->lexeme, len + 1);
    } else {
        t->lexeme = NULL;
    }

    return t;
}

static void eat_eol_o(){
    while(current_token.type == TOK_EOL){
        next_token();
    }
}

static void eat_eol_m(void) {
    if (current_token.type != TOK_EOL ) {
        error_exit(2, "Syntax error: expected end-of-line\n");
    }
    while (current_token.type == TOK_EOL) {
        next_token();
    }
}


ASTNode *parser_prog(){

    next_token();

    ASTNode *root = ast_new(AST_PROGRAM,NULL);

    ASTNode *prolog = parser_prolog();
    ASTNode *class = parser_class_def();

    ast_add_child(root,prolog);
    ast_add_child(root,class);

    eat_eol_o();

    expect(TOK_EOF);
    return root;
}

ASTNode *parser_prolog(){

    ASTNode *prolog = ast_new(AST_PROLOG,NULL);
    if (!is_keyword("import")){
        error_exit(2,"expected 'import' at the start of program \n");
    }
    next_token();
    
    eat_eol_o();
    
    if (current_token.type != TOK_STRING || !current_token.lexeme
        || strcmp(current_token.lexeme, "ifj25") != 0){
            error_exit(2 ,"expected 'ifj25' at the start of program \n");
        }
    ast_add_child(prolog,ast_new(AST_LITERAL,copy_token(&current_token)));
    next_token();

    if (!is_keyword("for")){
        error_exit(2,"expected 'for' after import \n");
    }
    next_token();
    eat_eol_o();

    if (!is_keyword("Ifj")){
        error_exit(2,"expected 'Ifj' after for \n");
    }
    ast_add_child(prolog,ast_new(AST_IDENTIFIER,copy_token(&current_token)));
    next_token();
    eat_eol_m();
    return prolog;
}

ASTNode *parser_class_def(){
    ASTNode *class_def = ast_new(AST_CLASS,NULL);
    ASTNode *fs = ast_new(AST_FUNCTION_S,NULL);

    if (!is_keyword("class")){
        error_exit(2 ,"expected 'class' at the start of class %s \n",tok2symbol(current_token.type));
        printf("%d",current_token.type);
    }
    next_token();

    if (current_token.type != TOK_IDENTIFIER ||
        !current_token.lexeme ||
        strcmp(current_token.lexeme, "Program") != 0) {
        error_exit(2,"expected class name 'Program' after class\n");
    }

    ast_add_child(class_def,ast_new(AST_IDENTIFIER,copy_token(&current_token)));

    next_token();
    expect(TOK_LBRACE);
    eat_eol_m();

    

    fs = parser_function_defs();
    ast_add_child(class_def ,fs);

    expect(TOK_RBRACE);

    return class_def;
}

ASTNode *parser_function_defs(){
    ASTNode *functions = ast_new(AST_FUNCTION_S,NULL);
    while(is_keyword("static")){
        ASTNode *f = parser_function_def();
        ast_add_child(functions,f);
    }
    return functions;
}

ASTNode *parser_function_def(){
    ASTNode *f = ast_new(AST_FUNCTION_DEF,NULL);

    if (!is_keyword("static")){
        error_exit(2,"expected 'static' at the start of function\n");
    }
    next_token();
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected function name after 'static'\n");
    }
    ast_add_child(f,ast_new(AST_IDENTIFIER,copy_token(&current_token)));
    next_token();

    ASTNode *f_kind = parser_function_kind();
    ast_add_child(f,f_kind);

    return f;
}
ASTNode *parser_function_kind(void) {
    switch (current_token.type) {

        case TOK_LPAREN:
            return parser_function_pick();

        case TOK_LBRACE:
            return parser_getter_pick();

        case TOK_ASSIGN:
            return parser_setter_pick();

        default:
            error_exit(2, "Syntax error: expected '(', '{' or '=' after function name");
    }
    return NULL;
    
}

ASTNode *parser_function_pick(){
    ASTNode *f_pick = ast_new(AST_FUNCTION,NULL);

    expect(TOK_LPAREN);
    param_list();
    expect(TOK_RPAREN);

    ASTNode *blok = block();
    ast_add_child(f_pick,blok);

    eat_eol_m();

    return f_pick;
}

ASTNode *parser_getter_pick(){
    ASTNode *f_get = ast_new(AST_GETTER,NULL);
    ASTNode *blok = block();
    ast_add_child(f_get,blok);
    eat_eol_m();

    return f_get;
}

ASTNode *parser_setter_pick(){
    ASTNode *f_set = ast_new(AST_SETTER,NULL);
    expect(TOK_ASSIGN);
    expect(TOK_LPAREN);
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected setter name after left parentace\n");
    }
    ast_add_child(f_set,ast_new(AST_IDENTIFIER,copy_token(&current_token)));
    next_token();

    expect(TOK_RPAREN);
    ASTNode *blok = block();
    ast_add_child(f_set,blok);    

    eat_eol_m();
    
    return f_set;
}

ASTNode *param_list(){
    ASTNode *param_list = ast_new(AST_PARAM_LIST,NULL);

    if (current_token.type == TOK_RPAREN) {
        return param_list;
    }   
    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected setter id after 'static'\n");
    }
    ast_add_child(param_list,ast_new(AST_IDENTIFIER,copy_token(&current_token)));

    next_token();

    if (current_token.type == TOK_COMMA){
    
    param_more(param_list);
    
    }

    return param_list;
}
void param_more(ASTNode *list){
    
    if (current_token.type != TOK_COMMA) {
        return; 
    }

    expect(TOK_COMMA);
    eat_eol_o();

    if (current_token.type != TOK_IDENTIFIER) {
        error_exit(2,"expected setter id after 'static'\n");
    }
    ast_add_child(list,ast_new(AST_IDENTIFIER,copy_token(&current_token)));

    next_token();

    
    param_more(list);

}

ASTNode *block(){
    ASTNode *blok = ast_new(AST_BLOCK,NULL);
    
    expect(TOK_LBRACE);
    eat_eol_m();
    
    parser_statements(blok);
    
    expect(TOK_RBRACE);
    
    return blok;
}

void parser_statements(ASTNode *blok) {
    
    while (
        is_keyword("var") ||
        is_keyword("return") ||
        is_keyword("if") ||
        is_keyword("while") || current_token.type == TOK_IDENTIFIER ||
        current_token.type == TOK_GID) {
            parser_statement(blok);

        }

}
    
    typedef enum {
        KW_NONE = 0,
        KW_VAR,
        KW_RETURN,
        KW_IF,
        KW_WHILE,
        KW_ELSE     // ← toto si odstránil
    } KeywordKind;
    
    
    static KeywordKind get_keyword(void) {
        if (current_token.type != TOK_KEYWORD)
        return KW_NONE;
        
        if (strcmp(current_token.lexeme, "var") == 0) return KW_VAR;
        if (strcmp(current_token.lexeme, "return") == 0) return KW_RETURN;
        if (strcmp(current_token.lexeme, "if") == 0) return KW_IF;
        if (strcmp(current_token.lexeme, "while") == 0) return KW_WHILE;
        if (strcmp(current_token.lexeme, "else") == 0) return KW_ELSE;   // ← DOPLNIŤ
        
        return KW_NONE;
    }

//-------------------------------------
//   ABSTRACT STATEMENT DISPATCHER
//-------------------------------------
void parser_statement(ASTNode *blok)
{
    if (current_token.type == TOK_IDENTIFIER ||
        current_token.type == TOK_GID)
    {
        ASTNode *sid = statement_sid();
        ast_add_child(blok, sid);
        return;
    }

    switch (get_keyword()) {
        case KW_VAR: {
            ASTNode *v = statement_var();
            ast_add_child(blok, v);
            return;
        }
        case KW_RETURN: {
            ASTNode *r = statement_return();
            ast_add_child(blok, r);
            return;
        }
        case KW_IF: {
            
            ASTNode *else_node = statement_if();

            ASTNode *ifnode = else_node->children[0]; 
            ASTNode *elsenode = else_node->children[1]; 

            ast_add_child(blok, ifnode);
            ast_add_child(blok, elsenode);

            return;
        }
        case KW_WHILE: {
            ASTNode *wh = statement_while();
            ast_add_child(blok, wh);
            return;
        }
        case KW_ELSE:
            error_exit(2, "unexpected 'else'\n");
            return;
            
        default:
            error_exit(2, "unexpected 'else'\n");
            return;
    }

    error_exit(2, "Syntax error: unexpected token at start of statement\n");
}



//-------------------------------------
//   VAR DECLARATION
//-------------------------------------
ASTNode *statement_var(void)
{
    next_token(); // skip 'var'

    if (current_token.type != TOK_IDENTIFIER)
        error_exit(2, "expected identifier after 'var'\n");

    ASTNode *var = ast_new(AST_VAR_DECL, copy_token(&current_token));
    next_token();

    ASTNode *tail = var_tail();
    if (tail)
        ast_add_child(var, tail);

    return var;
}



//-------------------------------------
//   RETURN
//-------------------------------------
ASTNode *statement_return()
{
    next_token(); // skip 'return'

    ASTNode *ret = ast_new(AST_RETURN, NULL);
    ASTNode *expr = return_tail();

    if (expr)
        ast_add_child(ret, expr);

    return ret;
}


//-------------------------------------
//   IF
//-------------------------------------
ASTNode *statement_if()
{
    next_token(); // skip 'if'

    // create wrapper
    ASTNode *wrap = ast_new(AST_EXPR, NULL); // dummy wrapper

    // IF node
    ASTNode *ifnode = ast_new(AST_IF, NULL);

    expect(TOK_LPAREN);
    ASTNode *cond = parse_expr();
    expect(TOK_RPAREN);
    ast_add_child(ifnode, cond);

    ASTNode *then_blk = block();
    ast_add_child(ifnode, then_blk);

    if (!is_keyword("else"))
        error_exit(2, "expected 'else' after if-block\n");

    next_token();

    // ELSE node
    ASTNode *elsen = ast_new(AST_ELSE, NULL);
    ASTNode *else_blk = block();
    ast_add_child(elsen, else_blk);

    eat_eol_m();

    // now wrap both as children
    ast_add_child(wrap, ifnode);
    ast_add_child(wrap, elsen);

    return wrap;
}





//-------------------------------------
//   WHILE
//-------------------------------------
ASTNode *statement_while()
{
    next_token(); // skip 'while'

    expect(TOK_LPAREN);
    ASTNode *cond = parse_expr();
    expect(TOK_RPAREN);

    ASTNode *body = block();
    eat_eol_m();

    ASTNode *wn = ast_new(AST_WHILE, NULL);
    ast_add_child(wn, cond);
    ast_add_child(wn, body);

    return wn;
}



ASTNode *statement_sid()
{
    ASTNode *idnode;

    if (current_token.type == TOK_IDENTIFIER) {
        idnode = ast_new(AST_IDENTIFIER, copy_token(&current_token));
    }
    else if (current_token.type == TOK_GID) {
        idnode = ast_new(AST_GID, copy_token(&current_token));
    }
    else {
        error_exit(2, "internal parser error in statement_sid()");
    }

    next_token();

    ASTNode *tail = id_tail(idnode->token);

    if (tail)
        return tail;

    return idnode;
}



//-------------------------------------
//   VAR TAIL (= expr)
//-------------------------------------
ASTNode *var_tail()
{
    if (current_token.type == TOK_ASSIGN) {
        next_token();

        ASTNode *expr = parse_expr();
        eat_eol_m();

        ASTNode *assign = ast_new(AST_ASSIGN, NULL);
        ast_add_child(assign, expr);

        return assign;
    }

    eat_eol_m();
    return NULL;
}



//-------------------------------------
//   ID TAIL (= expr | ( args ))
//-------------------------------------
ASTNode *id_tail(Token *id)
{
    if (current_token.type == TOK_ASSIGN) {
        next_token();

        ASTNode *expr = parse_expr();
        eat_eol_m();

        ASTNode *assign = ast_new(AST_ASSIGN, copy_token(id));
        ast_add_child(assign, expr);
        return assign;
    }

    if (current_token.type == TOK_LPAREN) {
        next_token();

        ASTNode *call = ast_new(AST_CALL, copy_token(id));
        arg_list(call);

        expect(TOK_RPAREN);
        eat_eol_m();

        return call;
    }

    eat_eol_m();
    return NULL;
}




//-------------------------------------
//   RETURN TAIL
//-------------------------------------
ASTNode *return_tail()
{
    if (starts_expr(current_token)) {
        ASTNode *expr = parse_expr();
        eat_eol_m();
        return expr;
    }

    eat_eol_m();
    return NULL;
}



//-------------------------------------
//   ARGUMENT LIST
//-------------------------------------
void arg_list(ASTNode *call)
{
    if (current_token.type == TOK_RPAREN)
        return;

    ASTNode *expr = parse_expr();
    ast_add_child(call, expr);

    arg_more(call);
}

void arg_more(ASTNode *call)
{
    if (current_token.type != TOK_COMMA)
        return;

    expect(TOK_COMMA);
    eat_eol_o();

    ASTNode *expr = parse_expr();
    ast_add_child(call, expr);

    arg_more(call);
}



//-------------------------------------
//   FAKE EXPR  (placeholder until precedence parser)
//-------------------------------------
ASTNode *parse_expr()
{
    if (!starts_expr(current_token))
        error_exit(2, "expected expression\n");

    ASTNode *expr = ast_new(AST_EXPR, copy_token(&current_token));
    next_token();

    return expr;
}

static int starts_expr(Token t)
{
    switch (t.type) {
        case TOK_IDENTIFIER:
        case TOK_GID:
        case TOK_INT:
        case TOK_FLOAT:
        case TOK_HEX:
        case TOK_STRING:
        case TOK_LPAREN:
            return 1;

        default:
            return 0;
    }
}


