//SPUSTENIE: gcc -std=c11 -Wall -Wextra -g psa_tests.c parse_expr.c psa_stack.c ast.c     err.c     -o psa_test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse_expr.h"
#include "ast.h"
#include "scanner.h"
#include "psa_stack.h"

/******************************************************************
 * 0. Farby pre PASS / FAIL
 ******************************************************************/
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RESET   "\033[0m"

/******************************************************************
 * 1. Falošný scanner_next – berie tokeny z globálneho poľa
 ******************************************************************/

static Token *g_tokens = NULL;
static size_t g_token_count = 0;
static size_t g_token_index = 0;

Token scanner_next(void)
{
    if (g_token_index < g_token_count)
        return g_tokens[g_token_index++];

    Token eof = { .type = TOK_EOF, .lexeme = NULL };
    return eof;
}

static void set_tokens(Token *tokens, size_t count)
{
    g_tokens = tokens;
    g_token_count = count;
    g_token_index = 1;
}

/******************************************************************
 * 2. Utility
 ******************************************************************/

static char *dupstr(const char *s)
{
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

static Token make_tok(TokenType type, const char *lex)
{
    Token t;
    t.type = type;
    t.lexeme = lex ? dupstr(lex) : NULL;
    return t;
}

static const char *tokstr(TokenType t)
{
    switch (t) {
        case TOK_INT:        return "TOK_INT";
        case TOK_FLOAT:      return "TOK_FLOAT";
        case TOK_HEX:        return "TOK_HEX";
        case TOK_STRING:     return "TOK_STRING";
        case TOK_IDENTIFIER: return "TOK_IDENTIFIER";
        case TOK_GID:        return "TOK_GID";
        case TOK_PLUS:       return "TOK_PLUS";
        case TOK_MINUS:      return "TOK_MINUS";
        case TOK_STAR:       return "TOK_STAR";
        case TOK_SLASH:      return "TOK_SLASH";
        case TOK_LT:         return "TOK_LT";
        case TOK_LE:         return "TOK_LE";
        case TOK_GT:         return "TOK_GT";
        case TOK_GE:         return "TOK_GE";
        case TOK_EQ:         return "TOK_EQ";
        case TOK_NE:         return "TOK_NE";
        case TOK_LPAREN:     return "TOK_LPAREN";
        case TOK_RPAREN:     return "TOK_RPAREN";
        case TOK_KEYWORD:    return "TOK_KEYWORD";
        case TOK_EOF:        return "TOK_EOF";
        default:             return "TOK_OTHER";
    }
}

static const char *aststr(int t)
{
    switch (t) {
        case AST_LITERAL:    return "AST_LITERAL";
        case AST_IDENTIFIER: return "AST_IDENTIFIER";
        case AST_EXPR:       return "AST_EXPR";
        case AST_CALL:       return "AST_CALL";
        default:             return "AST_OTHER";
    }
}

static const char *psastr(PsaResult r)
{
    switch (r) {
        case PSA_OK:           return "PSA_OK";
        case PSA_ERR_SYNTAX:   return "PSA_ERR_SYNTAX";
        case PSA_ERR_INTERNAL: return "PSA_ERR_INTERNAL";
        default:               return "PSA_UNKNOWN";
    }
}

static void print_root(ASTNode *root)
{
    if (!root) {
        printf("root=NULL");
        return;
    }
    printf("root.type=%s", aststr(root->type));
    if (root->token)
        printf(", root.token.type=%s", tokstr(root->token->type));
    else
        printf(", root.token=NULL");
}

static PsaResult run_psa(Token *tokens, size_t n, ASTNode **root)
{
    set_tokens(tokens, n);
    Token next;
    *root = NULL;
    return psa_parse_expression(tokens[0], &next, root);
}

/******************************************************************
 * 3. Test helper makro
 ******************************************************************/

#define RUN_TEST(func) \
    do { total++; passed += func(); printf("\n"); } while (0)

/******************************************************************
 * 4. Precedenčné testy – základné
 ******************************************************************/

static int test_prec_1_plus_2_mul_3(void)
{
    const char *name = "prec_1_plus_2_mul_3";
    const char *expr = "1 + 2 * 3";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_PLUS";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_PLUS) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_paren_1_plus_2_mul_3(void)
{
    const char *name = "prec_paren_1_plus_2_mul_3";
    const char *expr = "(1 + 2) * 3";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_STAR";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_STAR) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_rel_vs_add_mul(void)
{
    const char *name = "prec_rel_vs_add_mul";
    const char *expr = "1 + 2 < 3 * 4";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_LT";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "4"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_LT) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_is_operator(void)
{
    const char *name = "prec_is_operator";
    const char *expr = "1 + 2 is Int";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_KEYWORD";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Int"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_KEYWORD) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_eq_vs_add_mul(void)
{
    const char *name = "prec_eq_vs_add_mul";
    const char *expr = "1 + 2 * 3 == 7";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_INT, "7"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_is_vs_rel(void)
{
    const char *name = "prec_is_vs_rel";
    const char *expr = "1 < 2 is Bool";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_KEYWORD";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Bool"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_KEYWORD) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

/******************************************************************
 * 4b. Ďalšie precedenčné testy (na 40+ testov)
 ******************************************************************/

static int test_prec_mul_div_assoc(void)
{
    const char *name = "prec_mul_div_assoc";
    const char *expr = "1 * 2 / 3";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_SLASH";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_SLASH, "/"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }

    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_SLASH) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_add_chain(void)
{
    const char *name = "prec_add_chain";
    const char *expr = "1 + 2 - 3";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_MINUS";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_MINUS, "-"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_MINUS) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_rel_eq_chain(void)
{
    const char *name = "prec_rel_eq_chain";
    const char *expr = "1 < 2 == 1";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }

    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_paren_nested_eq(void)
{
    const char *name = "prec_paren_nested_eq";
    const char *expr = "((1 + 2) * 3) == 9";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_INT, "9"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }

    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_literal_int(void)
{
    const char *name = "prec_literal_int";
    const char *expr = "42";
    const char *expected = "root.type=AST_LITERAL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "42"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_LITERAL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_literal_string(void)
{
    const char *name = "prec_literal_string";
    const char *expr = "\"ahoj\"";
    const char *expected = "root.type=AST_LITERAL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_STRING, "ahoj"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_LITERAL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_identifier_only(void)
{
    const char *name = "prec_identifier_only";
    const char *expr = "x";
    const char *expected = "root.type=AST_IDENTIFIER";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "x"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_IDENTIFIER) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_is_with_paren(void)
{
    const char *name = "prec_is_with_paren";
    const char *expr = "(1 + 2) is Int";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_KEYWORD";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Int"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_KEYWORD) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_prec_eq_with_paren(void)
{
    const char *name = "prec_eq_with_paren";
    const char *expr = "(1+2) == (3-1)";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_MINUS, "-"),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

/******************************************************************
 * 5. FUNEXP testy – základ
 ******************************************************************/

static int test_funexp_foo_empty(void)
{
    const char *name = "funexp_foo_empty";
    const char *expr = "foo()";
    const char *expected = "root.type=AST_CALL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_CALL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_foo_1_plus_2(void)
{
    const char *name = "funexp_foo_1_plus_2";
    const char *expr = "foo(1 + 2)";
    const char *expected = "root.type=AST_CALL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_CALL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_in_expression(void)
{
    const char *name = "funexp_in_expression";
    const char *expr = "1 + foo(2 * 3)";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_PLUS";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_PLUS) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_in_relation(void)
{
    const char *name = "funexp_in_relation";
    const char *expr = "foo(1+2) < 10";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_LT";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_INT, "10"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_LT) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_in_is(void)
{
    const char *name = "funexp_in_is";
    const char *expr = "foo() is Int";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_KEYWORD";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Int"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_KEYWORD) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_on_both_sides_of_eq(void)
{
    const char *name = "funexp_on_both_sides_of_eq";
    const char *expr = "foo(1) == foo(2)";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_nested_funexp(void)
{
    const char *name = "nested_funexp";
    const char *expr = "foo(bar(1))";
    const char *expected = "root.type=AST_CALL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_IDENTIFIER, "bar"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      ");
    if (r != PSA_OK) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_CALL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

/******************************************************************
 * 5b. Ďalšie FUNEXP testy
 ******************************************************************/

static int test_funexp_nested_in_expression(void)
{
    const char *name = "funexp_nested_in_expression";
    const char *expr = "1 + foo(bar(1))";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_PLUS";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_IDENTIFIER, "bar"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_PLUS) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_two_calls_add(void)
{
    const char *name = "funexp_two_calls_add";
    const char *expr = "foo(1) + foo(2)";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_PLUS";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_PLUS) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_call_in_is_chain(void)
{
    const char *name = "funexp_call_in_is_chain";
    const char *expr = "foo(1) is Int == true";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_EQ";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Int"),
        make_tok(TOK_EQ, "=="),
        make_tok(TOK_KEYWORD, "true"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_EQ) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_call_left_rel(void)
{
    const char *name = "funexp_call_left_rel";
    const char *expr = "1 < foo(2) * 3";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_LT";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_LT) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_call_only_bar(void)
{
    const char *name = "funexp_call_only_bar";
    const char *expr = "bar(1+2*3)";
    const char *expected = "root.type=AST_CALL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "bar"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_STAR, "*"),
        make_tok(TOK_INT, "3"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }
    print_root(root);
    printf("\n");

    if (root->type == AST_CALL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_funexp_paren_call_sum_is(void)
{
    const char *name = "funexp_paren_call_sum_is";
    const char *expr = "(foo(1) + 2) is Int";
    const char *expected = "root.type=AST_EXPR, root.token.type=TOK_KEYWORD";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_IDENTIFIER, "Int"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);
    printf("Result:      ");

    if (r != PSA_OK || !root) {
        printf("PsaResult=%s, root=NULL\n", psastr(r));
        printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
        return 0;
    }

    print_root(root);
    printf("\n");

    if (root->type == AST_EXPR && root->token && root->token->type == TOK_KEYWORD) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

/******************************************************************
 * 6. Chybové stavy PSA
 ******************************************************************/

static int test_error_empty_expression(void)
{
    const char *name = "error_empty_expression";
    const char *expr = "<EOF>";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = { make_tok(TOK_EOF, NULL) };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_double_identifier(void)
{
    const char *name = "error_double_identifier";
    const char *expr = "a b";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "a"),
        make_tok(TOK_IDENTIFIER, "b"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_trailing_operator(void)
{
    const char *name = "error_trailing_operator";
    const char *expr = "1 +";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_missing_rparen_simple(void)
{
    const char *name = "error_missing_rparen_simple";
    const char *expr = "(1 + 2";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_missing_rparen_funexp(void)
{
    const char *name = "error_missing_rparen_funexp";
    const char *expr = "foo(1+2";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_leading_operator(void)
{
    const char *name = "error_leading_operator";
    const char *expr = "+ 1";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }
    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_only_rparen(void)
{
    const char *name = "error_only_rparen";
    const char *expr = ")";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = { make_tok(TOK_RPAREN, ")"), make_tok(TOK_EOF, NULL) };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_only_lparen(void)
{
    const char *name = "error_only_lparen";
    const char *expr = "(";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = { make_tok(TOK_LPAREN, "("), make_tok(TOK_EOF, NULL) };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_call_missing_rparen_no_arg(void)
{
    const char *name = "error_call_missing_rparen_no_arg";
    const char *expr = "foo(";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "foo"),
        make_tok(TOK_LPAREN, "("),
        make_tok(TOK_EOF, NULL)
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_double_operator_plus(void)
{
    const char *name = "error_double_operator_plus";
    const char *expr = "1 ++ 2";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_PLUS, "+"),
        make_tok(TOK_INT, "2"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_rel_no_right(void)
{
    const char *name = "error_rel_no_right";
    const char *expr = "1 <";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_INT, "1"),
        make_tok(TOK_LT, "<"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_leading_is(void)
{
    const char *name = "error_leading_is";
    const char *expr = "is 1";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_KEYWORD, "is"),
        make_tok(TOK_INT, "1"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

static int test_error_id_then_rparen(void)
{
    const char *name = "error_id_then_rparen";
    const char *expr = "a )";
    const char *expected = "PsaResult=PSA_ERR_SYNTAX, root=NULL";

    printf("Test_name:   %s\n", name);
    printf("Expression:  %s\n", expr);
    printf("Expected:    %s\n", expected);

    Token t[] = {
        make_tok(TOK_IDENTIFIER, "a"),
        make_tok(TOK_RPAREN, ")"),
        make_tok(TOK_EOF, NULL),
    };

    ASTNode *root;
    PsaResult r = run_psa(t, sizeof(t)/sizeof(t[0]), &root);

    printf("Result:      PsaResult=%s, ", psastr(r));
    print_root(root);
    printf("\n");

    if (r == PSA_ERR_SYNTAX && root == NULL) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET "\n");
        return 1;
    }

    printf(COLOR_RED "[FAIL]" COLOR_RESET "\n");
    return 0;
}

/******************************************************************
 * 7. main – spustí všetky testy
 ******************************************************************/

int main(void)
{
    int total = 0;
    int passed = 0;

    printf("Running Tests for Parse_expresions:\n\n");

    /* Precedenčné testy – základ */
    RUN_TEST(test_prec_1_plus_2_mul_3);
    RUN_TEST(test_prec_paren_1_plus_2_mul_3);
    RUN_TEST(test_prec_rel_vs_add_mul);
    RUN_TEST(test_prec_is_operator);
    RUN_TEST(test_prec_eq_vs_add_mul);
    RUN_TEST(test_prec_is_vs_rel);

    /* Precedenčné testy – rozšírené */
    RUN_TEST(test_prec_mul_div_assoc);
    RUN_TEST(test_prec_add_chain);
    RUN_TEST(test_prec_rel_eq_chain);
    RUN_TEST(test_prec_paren_nested_eq);
    RUN_TEST(test_prec_literal_int);
    RUN_TEST(test_prec_literal_string);
    RUN_TEST(test_prec_identifier_only);
    RUN_TEST(test_prec_is_with_paren);
    RUN_TEST(test_prec_eq_with_paren);

    /* FUNEXP testy – základ */
    RUN_TEST(test_funexp_foo_empty);
    RUN_TEST(test_funexp_foo_1_plus_2);
    RUN_TEST(test_funexp_in_expression);
    RUN_TEST(test_funexp_in_relation);
    RUN_TEST(test_funexp_in_is);
    RUN_TEST(test_funexp_on_both_sides_of_eq);
    RUN_TEST(test_nested_funexp);

    /* FUNEXP testy – rozšírené */
    RUN_TEST(test_funexp_nested_in_expression);
    RUN_TEST(test_funexp_two_calls_add);
    RUN_TEST(test_funexp_call_in_is_chain);
    RUN_TEST(test_funexp_call_left_rel);
    RUN_TEST(test_funexp_call_only_bar);
    RUN_TEST(test_funexp_paren_call_sum_is);

    /* Chybové stavy PSA – základ */
    RUN_TEST(test_error_empty_expression);
    RUN_TEST(test_error_double_identifier);
    RUN_TEST(test_error_trailing_operator);
    RUN_TEST(test_error_missing_rparen_simple);
    RUN_TEST(test_error_missing_rparen_funexp);
    RUN_TEST(test_error_leading_operator);

    /* Chybové stavy PSA – rozšírené */
    RUN_TEST(test_error_only_rparen);
    RUN_TEST(test_error_only_lparen);
    RUN_TEST(test_error_call_missing_rparen_no_arg);
    RUN_TEST(test_error_double_operator_plus);
    RUN_TEST(test_error_rel_no_right);
    RUN_TEST(test_error_leading_is);
    RUN_TEST(test_error_id_then_rparen);

    printf("Summary: ");
    if (passed == total) {
        printf(COLOR_GREEN "%d/%d tests PASSED" COLOR_RESET "\n", passed, total);
    } else {
        printf(COLOR_RED "%d/%d tests PASSED" COLOR_RESET "\n", passed, total);
    }

    return (passed == total) ? 0 : 1;
}
