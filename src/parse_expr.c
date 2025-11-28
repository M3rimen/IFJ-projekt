/**
 * @file parse_expr.c
 * @brief Expression parser using operator-precedence parsing (PSA) with AST construction.
 *
 * This module parses expressions using a precedence table and an internal PSA stack.
 * It supports:
 *  - arithmetic operators (+, -, *, /)
 *  - relational operators (<, <=, >, >=)
 *  - equality operators (==, !=)
 *  - type check operator ("is")
 *  - parentheses
 *  - identifiers and literals
 *  - simple function calls as expressions (foo(), foo(expr))
 */

#include "parse_expr.h"
#include "psa_stack.h"
#include "scanner.h"
#include "err.h"

#include <string.h>
#include <stdlib.h>

/**
 * @brief Operator precedence table for PSA.
 *
 * Indices correspond to values of ::PrecedenceGroup.
 * Cells contain a ::PrecedenceRelation describing the relation
 * between the operator on the stack (row) and the current input (column).
 *
 * Rows/columns (in order):
 *  - GRP_MUL_DIV (MD)
 *  - GRP_ADD_SUB (AS)
 *  - GRP_REL     (REL)
 *  - GRP_IS      (IS)
 *  - GRP_EQ      (EQ)
 *  - GRP_ID      (ID)
 *  - GRP_LPAREN  ('(')
 *  - GRP_RPAREN  (')')
 *  - GRP_EOF     ('$' – end marker)
 */
PrecedenceRelation prec_table[9][9] = {
    //        MD     AS     REL    IS     EQ     ID     (      )      $
    /*MD*/   {GT,    GT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*AS*/   {LT,    GT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*REL*/  {LT,    LT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*IS*/   {LT,    LT,    LT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*EQ*/   {LT,    LT,    LT,    LT,    GT,    LT,    LT,    GT,    GT},
    /*ID*/   {GT,    GT,    GT,    GT,    GT,    UD,    LT,    GT,    GT},
    /*(*/    {LT,    LT,    LT,    LT,    LT,    LT,    LT,    EQ,    UD},
    /*)*/    {GT,    GT,    GT,    GT,    GT,    UD,    UD,    GT,    GT},
    /*$*/    {LT,    LT,    LT,    LT,    LT,    LT,    LT,    UD,    EQ}
};

/**
 * @brief Map a lexical token to a precedence group used by the PSA.
 *
 * @param tok Pointer to the token to classify.
 * @return Corresponding ::PrecedenceGroup for the token.
 */
PrecedenceGroup token_to_group(const Token *tok)
{
    switch (tok->type) {
    case TOK_STAR:
    case TOK_SLASH:
        return GRP_MUL_DIV;

    case TOK_PLUS:
    case TOK_MINUS:
        return GRP_ADD_SUB;

    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE:
        return GRP_REL;

    case TOK_EQ:
    case TOK_NE:
        return GRP_EQ;

    case TOK_LPAREN:
        return GRP_LPAREN;
    case TOK_RPAREN:
        return GRP_RPAREN;

    case TOK_EOF:
        return GRP_EOF;

    case TOK_IDENTIFIER:
    case TOK_GID:
    case TOK_INT:
    case TOK_FLOAT:
    case TOK_HEX:
    case TOK_STRING:
        return GRP_ID;

    case TOK_KEYWORD:
        if (tok->lexeme && strcmp(tok->lexeme, "is") == 0)
            return GRP_IS;
        return GRP_ID;

    default:
        return GRP_EOF;
    }
}

/**
 * @brief Check whether a token type marks the end of an expression.
 *
 * Currently only TOK_EOF is treated as an end-of-expression token. Other
 * syntactic end markers (semicolon, EOL) are handled explicitly in
 * psa_parse_expression().
 *
 * @param t Token type to test.
 * @return Non-zero if the token type marks the end of expression, 0 otherwise.
 */
static int is_expr_end_token(TokenType t)
{
    return (t == TOK_EOF);
}

/**
 * @brief Check whether the last seen token is an operator or left parenthesis.
 *
 * This function is used when handling line breaks to decide whether we
 * should continue reading tokens on the next line as part of the same
 * expression (e.g., after an operator).
 *
 * @param last_type     Type of the last token.
 * @param last_is_is_op Non-zero if the last token was keyword "is".
 * @return Non-zero if last token is considered an operator or '(', 0 otherwise.
 */
static int is_op_or_lparen(TokenType last_type, int last_is_is_op)
{
    if (last_is_is_op)
        return 1;

    switch (last_type) {
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE:
    case TOK_EQ:
    case TOK_NE:
    case TOK_LPAREN:
        return 1;
    default:
        return 0;
    }
}

/**
 * @brief Free dynamically allocated lexeme inside a token, if any.
 *
 * This helper does not free the token structure itself, only its
 * string payload and sets it to NULL.
 *
 * @param tok Pointer to the token whose lexeme should be freed.
 */
static inline void free_token_lexeme(Token *tok)
{
    if (tok && tok->lexeme) {
        free(tok->lexeme);
        tok->lexeme = NULL;
    }
}

/**
 * @brief Create an AST node for a given token.
 *
 * Depending on token type, returns:
 *  - AST_LITERAL for literals (numbers, strings),
 *  - AST_IDENTIFIER for identifiers,
 *  - AST_EXPR for operators or the "is" keyword,
 *  - NULL for unsupported tokens.
 *
 * The function calls ast_new(), passing the token pointer as its payload.
 *
 * @param tok Pointer to the token to convert.
 * @return Newly allocated ::ASTNode or NULL on unsupported token.
 */
static ASTNode *make_ast_node_for_token(const Token *tok)
{
    switch (tok->type) {
    case TOK_INT:
    case TOK_FLOAT:
    case TOK_HEX:
    case TOK_STRING:
        return ast_new(AST_LITERAL, (Token *)tok);

    case TOK_IDENTIFIER:
    case TOK_GID:
        return ast_new(AST_IDENTIFIER, (Token *)tok);

    case TOK_KEYWORD:
        if (tok->lexeme && strcmp(tok->lexeme, "is") == 0)
            return ast_new(AST_EXPR, (Token *)tok);
        return ast_new(AST_IDENTIFIER, (Token *)tok);

    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE:
    case TOK_EQ:
    case TOK_NE:
        return ast_new(AST_EXPR, (Token *)tok);

    default:
        return NULL;
    }
}

/**
 * @brief Reduce one handle from the PSA stack when precedence is GT.
 *
 * This function pops symbols from the stack until it encounters
 * a SYM_MARKER, collecting them into a handle (up to 4 items).
 *
 * It supports grammar-like patterns (in terms of stack symbols):
 *  - E -> ID
 *  - E -> ( E )
 *  - E -> E op E      (for op in {*,/, +,-, <,<=,>,>=, is, ==, !=})
 *  - FUNEXP: E -> E ( E )    (function call)
 *  - FUNEXP helper: () -> nonterm with no AST node (for foo())
 *
 * If @p build_ast is 0, only syntax is checked and a generic nonterminal
 * is pushed without creating AST nodes. Otherwise, appropriate AST nodes
 * are created and wired:
 *  - operators become AST_EXPR with left/right children,
 *  - function calls become AST_CALL nodes with callee and optional argument.
 *
 * @param build_ast Non-zero to construct AST, zero for syntax-only check.
 * @return PSA_OK on success, PSA_ERR_SYNTAX on grammar error,
 *         PSA_ERR_INTERNAL on unexpected stack/AST inconsistency.
 */
static PsaResult psa_reduce_handle(int build_ast)
{
    StackItem handle[4];
    int hlen = 0;

    while (1)
    {
        if (stack_size() == 0)
            return PSA_ERR_INTERNAL;

        StackItem it = stack_pop();

        if (it.kind == SYM_MARKER)
            break;

        if (hlen >= 4)
            return PSA_ERR_INTERNAL;

        handle[hlen++] = it;
    }

    // -------------------- syntax-only mode --------------------
    if (!build_ast)
    {
        // E -> ID
        if (hlen == 1 &&
            handle[0].kind == SYM_TERMINAL &&
            handle[0].group == GRP_ID)
        {
            // ok
        }
        // E -> ( E )
        else if (hlen == 3 &&
                 handle[0].kind == SYM_TERMINAL &&
                 handle[0].tok_type == TOK_RPAREN &&
                 handle[1].kind == SYM_NONTERM &&
                 handle[2].kind == SYM_TERMINAL &&
                 handle[2].tok_type == TOK_LPAREN)
        {
            // ok
        }
        // E -> E op E
        else if (hlen == 3 &&
                 handle[0].kind == SYM_NONTERM &&
                 handle[1].kind == SYM_TERMINAL &&
                 handle[2].kind == SYM_NONTERM)
        {
            PrecedenceGroup g = handle[1].group;
            if (!(g == GRP_MUL_DIV ||
                  g == GRP_ADD_SUB ||
                  g == GRP_REL     ||
                  g == GRP_IS      ||
                  g == GRP_EQ))
            {
                return PSA_ERR_SYNTAX;
            }
            // ok
        }
        // FUNEXP: E -> E ( E )  (handle je [E, ID])
        else if (hlen == 2 &&
                 handle[0].kind == SYM_NONTERM &&
                 handle[1].kind == SYM_TERMINAL &&
                 handle[1].group == GRP_ID)
        {
            // ok
        }
        // FUNEXP helper: () -> prázdny nonterm (pre foo())
        else if (hlen == 2 &&
                 handle[0].kind == SYM_TERMINAL &&
                 handle[0].tok_type == TOK_RPAREN &&
                 handle[1].kind == SYM_TERMINAL &&
                 handle[1].tok_type == TOK_LPAREN)
        {
            // ok – vytvoríme nonterm bez AST uzla
        }
        else {
            return PSA_ERR_SYNTAX;
        }

        stack_push_nonterm(TYPE_NONE, NULL);
        return PSA_OK;
    }

    // -------------------- AST-building mode --------------------
    ASTNode *new_node = NULL;

    // E -> ID
    if (hlen == 1 &&
        handle[0].kind == SYM_TERMINAL &&
        handle[0].group == GRP_ID)
    {
        new_node = handle[0].node;
        if (!new_node)
            return PSA_ERR_INTERNAL;
    }
    // E -> ( E )
    else if (hlen == 3 &&
             handle[0].kind == SYM_TERMINAL &&
             handle[0].tok_type == TOK_RPAREN &&
             handle[1].kind == SYM_NONTERM &&
             handle[2].kind == SYM_TERMINAL &&
             handle[2].tok_type == TOK_LPAREN)
    {
        new_node = handle[1].node;
        if (!new_node)
            return PSA_ERR_INTERNAL;
    }
    // E -> E op E
    else if (hlen == 3 &&
             handle[0].kind == SYM_NONTERM &&
             handle[1].kind == SYM_TERMINAL &&
             handle[2].kind == SYM_NONTERM)
    {
        PrecedenceGroup g = handle[1].group;
        if (!(g == GRP_MUL_DIV ||
              g == GRP_ADD_SUB ||
              g == GRP_REL     ||
              g == GRP_IS      ||
              g == GRP_EQ))
        {
            return PSA_ERR_SYNTAX;
        }

        ASTNode *right = handle[0].node;
        ASTNode *op    = handle[1].node;
        ASTNode *left  = handle[2].node;

        if (!op || !left || !right)
            return PSA_ERR_INTERNAL;

        ast_add_child(op, left);
        ast_add_child(op, right);
        new_node = op;
    }
    // FUNEXP: E -> E ( E )  (handle [E, ID])
    else if (hlen == 2 &&
             handle[0].kind == SYM_NONTERM &&
             handle[1].kind == SYM_TERMINAL &&
             handle[1].group == GRP_ID)
    {
        ASTNode *arg  = handle[0].node;  /**< expression inside parentheses (may be NULL for foo()) */
        ASTNode *func = handle[1].node;  /**< function identifier */

        if (!func)
            return PSA_ERR_INTERNAL;

        ASTNode *call = ast_new(AST_CALL,
                                func->token ? func->token : NULL);

        ast_add_child(call, func);
        if (arg)
            ast_add_child(call, arg); // for foo() arg == NULL → only callee child

        new_node = call;
    }
    // FUNEXP helper: () -> prázdny nonterm bez AST uzla
    else if (hlen == 2 &&
             handle[0].kind == SYM_TERMINAL &&
             handle[0].tok_type == TOK_RPAREN &&
             handle[1].kind == SYM_TERMINAL &&
             handle[1].tok_type == TOK_LPAREN)
    {
        // Create a nonterminal without AST node (used as "empty args" for foo()).
        stack_push_nonterm(TYPE_NONE, NULL);
        return PSA_OK;
    }
    else {
        return PSA_ERR_SYNTAX;
    }

    if (!new_node)
        return PSA_ERR_INTERNAL;

    stack_push_nonterm(TYPE_NONE, new_node);
    return PSA_OK;
}

/**
 * @brief Parse an expression using PSA and optionally build an AST.
 *
 * This is the main entry point for expression parsing. It:
 *  - initializes the PSA stack and pushes a bottom EOF terminal,
 *  - processes tokens starting from @p first,
 *  - uses the precedence table to decide between shift/reduce,
 *  - supports pseudo-EOF on semicolons/EOL to delimit expressions,
 *  - optionally returns the root AST node.
 *
 * Semantics of parameters:
 *  - @p first is the first token of the expression (already read).
 *  - @p out_next, if non-NULL, receives the first token *after*
 *    the parsed expression (e.g., semicolon or EOL).
 *  - @p out_ast, if non-NULL, triggers AST construction and receives
 *    the resulting root node on success.
 *
 * @param first    First token of expression (by value).
 * @param out_next Pointer to token receiving token after expression (may be NULL).
 * @param out_ast  Pointer to ASTNode* receiving root of expression AST (may be NULL).
 * @return PSA_OK on success,
 *         PSA_ERR_SYNTAX on syntax error,
 *         PSA_ERR_INTERNAL on internal consistency error.
 */
PsaResult psa_parse_expression(Token first, Token *out_next, ASTNode **out_ast)
{
    stack_init();

    int build_ast = (out_ast != NULL);

    if (out_ast)
        *out_ast = NULL;

    Token bottom_tok;
    bottom_tok.type   = TOK_EOF;
    bottom_tok.lexeme = NULL;
    stack_push_terminal(&bottom_tok, NULL);

    Token current = first;

    if (current.type == TOK_EOF ||
        current.type == TOK_SEMICOLON ||
        current.type == TOK_EOL)
    {
        return PSA_ERR_SYNTAX;
    }

    int use_pseudo_eof = 0;
    Token end_token;

    TokenType last_type = current.type;
    int last_is_is_op =
        (current.type == TOK_KEYWORD &&
         current.lexeme &&
         strcmp(current.lexeme, "is") == 0);

    while (1)
    {
        /* Handle EOLs: possible continuation of expression on next line. */
        if (!use_pseudo_eof && current.type == TOK_EOL) {

            if (is_op_or_lparen(last_type, last_is_is_op)) {
                do {
                    current = scanner_next();
                } while (current.type == TOK_EOL);
                continue;
            }

            Token la = scanner_next();

            if (la.type != TOK_EOF) {
                current = la;
                continue;
            }

            use_pseudo_eof = 1;
            end_token = current;
        }

        StackItem *top_term = stack_top_terminal();
        if (!top_term) {
            free_token_lexeme(&current);
            return PSA_ERR_INTERNAL;
        }

        PrecedenceGroup g_stack = top_term->group;
        PrecedenceGroup g_input;

        if (!use_pseudo_eof)
        {
            if (current.type == TOK_SEMICOLON) {
                Token la = scanner_next();

                if (la.type == TOK_EOL) {
                    use_pseudo_eof = 1;
                    end_token = la;
                    current   = la;
                } else {
                    use_pseudo_eof = 1;
                    end_token = current;
                }

                g_input = GRP_EOF;
            }
            else if (is_expr_end_token(current.type))
            {
                use_pseudo_eof = 1;
                end_token = current;
                g_input = GRP_EOF;
            }
            else
            {
                g_input = token_to_group(&current);
            }
        }
        else
        {
            g_input = GRP_EOF;
        }

        PrecedenceRelation rel = prec_table[g_stack][g_input];

        /* Special case: stack contains E and EOF on top, and we are at pseudo-EOF. */
        if (use_pseudo_eof && stack_is_eof_with_E_on_top())
        {
            if (rel == EQ)
            {
                if (out_next)
                    *out_next = end_token;

                if (build_ast) {
                    StackItem *top = stack_top();
                    if (!top || top->kind != SYM_NONTERM) {
                        free_token_lexeme(&current);
                        return PSA_ERR_INTERNAL;
                    }
                    *out_ast = top->node;
                }

                free_token_lexeme(&current);
                return PSA_OK;
            }
            else if (rel == GT)
            {
                PsaResult r = psa_reduce_handle(build_ast);
                if (r != PSA_OK) {
                    free_token_lexeme(&current);
                    return r;
                }
                continue;
            }
            else
            {
                free_token_lexeme(&current);
                return PSA_ERR_SYNTAX;
            }
        }

        /* Standard PSA decision: shift (<), equal (=), reduce (>) or error (UD). */
        switch (rel)
        {
        case LT:
        {
            stack_insert_marker_after_top_terminal();
            if (use_pseudo_eof) {
                free_token_lexeme(&current);
                return PSA_ERR_INTERNAL;
            }

            ASTNode *node = NULL;
            if (build_ast)
                node = make_ast_node_for_token(&current);

            stack_push_terminal(&current, node);

            free_token_lexeme(&current);

            last_type = current.type;
            last_is_is_op = (current.type == TOK_KEYWORD &&
                             current.lexeme &&
                             strcmp(current.lexeme, "is") == 0);

            current = scanner_next();
            break;
        }

        case EQ:
        {
            if (use_pseudo_eof) {
                free_token_lexeme(&current);
                return PSA_ERR_SYNTAX;
            }

            ASTNode *node = NULL;
            if (build_ast)
                node = make_ast_node_for_token(&current);

            stack_push_terminal(&current, node);

            free_token_lexeme(&current);

            last_type = current.type;
            last_is_is_op = (current.type == TOK_KEYWORD &&
                             current.lexeme &&
                             strcmp(current.lexeme, "is") == 0);

            current = scanner_next();
            break;
        }

        case GT:
        {
            PsaResult r = psa_reduce_handle(build_ast);
            if (r != PSA_OK) {
                free_token_lexeme(&current);
                return r;
            }
            break;
        }

        case UD:
        default:
            free_token_lexeme(&current);
            return PSA_ERR_SYNTAX;
        }
    }
}

/**
 * @brief Wrapper around psa_parse_expression() that exits on error.
 *
 * This helper is suitable for parts of the compiler where expression
 * parsing errors are fatal. It calls psa_parse_expression() and on:
 *  - PSA_OK: returns normally,
 *  - PSA_ERR_SYNTAX: calls error_exit(2, "Syntax error in expression"),
 *  - PSA_ERR_INTERNAL or anything else: calls error_exit(99, "...").
 *
 * @param first    First token of expression (by value).
 * @param out_next Pointer to token receiving token after expression (may be NULL).
 * @param out_ast  Pointer to ASTNode* receiving root of expression AST (may be NULL).
 */
void parse_expression_or_die(Token first, Token *out_next, ASTNode **out_ast)
{
    PsaResult r = psa_parse_expression(first, out_next, out_ast);

    if (r == PSA_OK) {
        return;
    }

    if (r == PSA_ERR_SYNTAX) {
        error_exit(2, "Syntax error in expression");
    } else if (r == PSA_ERR_INTERNAL) {
        error_exit(99, "Internal error in expression parser");
    } else {
        // fallback, ak by niekto neskôr pridal ďalší enum
        error_exit(99, "Unknown error in expression parser");
    }
}
