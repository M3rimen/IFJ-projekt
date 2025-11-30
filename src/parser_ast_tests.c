// parser_ast_tests.c
//
// Rozsiahle integračné testy: zdrojový kód -> scanner -> parser_prog -> AST,
// vrátane PSA výrazov (cez parse_expr).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "err.h"

#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RESET   "\033[0m"

/* ---------------- pomocné stringy ---------------- */

static const char *tokstr(TokenType t)
{
    switch (t) {
    case TOK_IDENTIFIER: return "IDENT";
    case TOK_GID:        return "GID";
    case TOK_KEYWORD:    return "KW";
    case TOK_INT:        return "INT";
    case TOK_FLOAT:      return "FLOAT";
    case TOK_HEX:        return "HEX";
    case TOK_STRING:     return "STRING";

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

    case TOK_LPAREN:    return "(";
    case TOK_RPAREN:    return ")";
    case TOK_LBRACE:    return "{";
    case TOK_RBRACE:    return "}";
    case TOK_COMMA:     return ",";
    case TOK_DOT:       return ".";
    case TOK_SEMICOLON: return "SEMICOLON";
    case TOK_EOL:       return "EOL";
    case TOK_EOF:       return "EOF";

    default:            return "?";
    }
}

static const char *aststr(AST_TYPE t)
{
    switch (t) {
    case AST_PROGRAM:      return "PROGRAM";
    case AST_PROLOG:       return "PROLOG";
    case AST_CLASS:        return "CLASS";
    case AST_FUNCTION_S:   return "FUNCTION_S";
    case AST_FUNCTION_DEF: return "FUNCTION_DEF";
    case AST_FUNCTION:     return "FUNCTION";
    case AST_GETTER:       return "GETTER";
    case AST_SETTER:       return "SETTER";
    case AST_FUNC_NAME:    return "FUNC_NAME";
    case AST_PARAM_LIST:   return "PARAM_LIST";
    case AST_BLOCK:        return "BLOCK";

    case AST_VAR_DECL:     return "VAR_DECL";
    case AST_ASSIGN:       return "ASSIGN";
    case AST_RETURN:       return "RETURN";
    case AST_IF:           return "IF";
    case AST_ELSE:         return "ELSE";
    case AST_WHILE:        return "WHILE";

    case AST_EXPR:         return "EXPR";
    case AST_LITERAL:      return "LITERAL";
    case AST_IDENTIFIER:   return "IDENT";
    case AST_GID:          return "GID";
    case AST_CALL:         return "CALL";

    default:               return "OTHER";
    }
}

/* ---------------- AST helper funkcie ---------------- */

static ASTNode *child(ASTNode *n, int idx)
{
    if (!n) return NULL;
    if (idx < 0 || idx >= n->child_count) return NULL;
    return n->children[idx];
}

static ASTNode *find_first(ASTNode *root, AST_TYPE type)
{
    if (!root) return NULL;
    if (root->type == type) return root;

    for (int i = 0; i < root->child_count; ++i) {
        ASTNode *r = find_first(root->children[i], type);
        if (r) return r;
    }
    return NULL;
}

/* ---------------- AST S-expression (len pre výrazové stromy) ---------------- */

static void print_sexpr(ASTNode *node);

static const char *op_lexeme(ASTNode *node)
{
    if (!node || !node->token)
        return "?";
    if (node->token->lexeme && node->token->lexeme[0] != '\0')
        return node->token->lexeme;
    return tokstr(node->token->type);
}

static void print_sexpr_children(ASTNode *node)
{
    for (int i = 0; i < node->child_count; ++i) {
        if (i > 0)
            printf(" ");
        print_sexpr(node->children[i]);
    }
}

static void print_sexpr(ASTNode *node)
{
    if (!node) {
        printf("null");
        return;
    }

    switch (node->type) {
    case AST_LITERAL:
    case AST_IDENTIFIER:
        if (node->token && node->token->lexeme)
            printf("%s", node->token->lexeme);
        else
            printf("%s", aststr(node->type));
        break;

    case AST_EXPR:
        if (node->child_count == 2) {
            printf("(%s ", op_lexeme(node));
            print_sexpr(child(node, 0));
            printf(" ");
            print_sexpr(child(node, 1));
            printf(")");
        } else {
            printf("(%s ", op_lexeme(node));
            print_sexpr_children(node);
            printf(")");
        }
        break;

    case AST_CALL:
        printf("(CALL ");
        print_sexpr_children(node);
        printf(")");
        break;

    default:
        printf("(%s ", aststr(node->type));
        print_sexpr_children(node);
        printf(")");
        break;
    }
}

/* ---------------- AST strom (ASCII) ---------------- */

static void print_ast_node_ascii(ASTNode *node, const char *prefix, int is_last)
{
    printf("%s", prefix);
    printf(is_last ? "└── " : "├── ");

    if (!node) {
        printf("(null)\n");
        return;
    }

    printf("%s", aststr(node->type));
    if (node->token) {
        printf(" [");
        printf("%s", tokstr(node->token->type));
        if (node->token->lexeme)
            printf(" '%s'", node->token->lexeme);
        printf("]");
    }
    printf("\n");

    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s",
             prefix, is_last ? "    " : "│   ");

    for (int i = 0; i < node->child_count; ++i) {
        int last_child = (i == node->child_count - 1);
        print_ast_node_ascii(node->children[i], new_prefix, last_child);
    }
}

static void print_ast_tree(ASTNode *root)
{
    printf("AST S-expr : ");
    if (root)
        print_sexpr(root);
    else
        printf("null");
    printf("\n");

    printf("AST STROM:\n");
    if (!root) {
        printf("  (null)\n");
        return;
    }
    print_ast_node_ascii(root, "  ", 1);
}

/* ---------------- bežný header + result testu ---------------- */

static void print_test_header(const char *name, const char *src, const char *expected)
{
    printf("Test_name:   %s\n", name);
    printf("Source:\n%s", src);
    size_t len = strlen(src);
    if (len == 0 || src[len - 1] != '\n')
        printf("\n");
    printf("Expected:    %s\n", expected);
}

static void print_result_line(int ok, const char *detail)
{
    printf("Result:      %s\n", detail ? detail : (ok ? "OK" : "Mismatch"));
    printf("%s[%s]%s\n",
           ok ? COLOR_GREEN : COLOR_RED,
           ok ? "PASS" : "FAIL",
           COLOR_RESET);
}

/* ---------------- wrapper: celý program z reťazca ---------------- */

static ASTNode *parse_program_from_string(const char *src)
{
    FILE *f = tmpfile();
    if (!f) {
        perror("tmpfile");
        exit(99);
    }

    fputs(src, f);
    rewind(f);

    scanner_init(f);
    ASTNode *root = parser_prog();

    fclose(f);
    return root;
}

/* ---------------- RUN_TEST makro ---------------- */

#define RUN_TEST(fn) \
    do { total++; passed += (fn)(); printf("\n"); } while (0)

/* ============================================================= */
/*                         TESTY                                 */
/* ============================================================= */

/*
 * 1) Program:
 *  return 1 + 2 * 3;
 */
static int test_parser_return_expr_1_plus_2_mul_3(void)
{
    const char *name = "parser_return_expr_1_plus_2_mul_3";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return 1 + 2 * 3;\n"
        "    }\n"
        "}\n";
    const char *expected =
        "RETURN -> EXPR '+', left=1, right=EXPR '*' (2*3)";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    if (root->type != AST_PROGRAM)
        ok = 0;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret)
        ok = 0;

    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token || expr->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(expr, 0) : NULL;
    ASTNode *R = ok ? child(expr, 1) : NULL;

    if (!L || !R)
        ok = 0;
    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;
    if (ok && (R->type != AST_EXPR || !R->token || R->token->type != TOK_STAR))
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 2) Program:
 *  var a = 1 + 2 * 3;
 */
static int test_parser_var_decl_expr(void)
{
    const char *name = "parser_var_decl_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var a = 1 + 2 * 3;\n"
        "    }\n"
        "}\n";
    const char *expected =
        "VAR_DECL a -> ASSIGN -> EXPR '+', right=EXPR '*'";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *var = find_first(root, AST_VAR_DECL);
    if (!var)
        ok = 0;

    ASTNode *assign = ok ? child(var, 0) : NULL;
    if (!assign || assign->type != AST_ASSIGN)
        ok = 0;

    ASTNode *expr = ok ? child(assign, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token || expr->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(expr, 0) : NULL;
    ASTNode *R = ok ? child(expr, 1) : NULL;

    if (!L || !R)
        ok = 0;
    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;
    if (ok && (R->type != AST_EXPR || !R->token || R->token->type != TOK_STAR))
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 3) Program:
 *  return foo(1+2);
 */
static int test_parser_return_fun_call_expr(void)
{
    const char *name = "parser_return_fun_call";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return foo(1+2);\n"
        "    }\n"
        "}\n";
    const char *expected =
        "RETURN -> CALL(foo, EXPR '+')";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret)
        ok = 0;

    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr)
        ok = 0;

    ASTNode *call = NULL;
    if (ok && expr->type == AST_CALL) {
        call = expr;
    } else if (ok && expr->type == AST_EXPR) {
        for (int i = 0; i < expr->child_count; ++i) {
            if (expr->children[i]->type == AST_CALL) {
                call = expr->children[i];
                break;
            }
        }
    }

    if (!call)
        ok = 0;

    if (ok) {
        ASTNode *fn = child(call, 0);
        if (!fn || fn->type != AST_IDENTIFIER ||
            !fn->token || fn->token->type != TOK_IDENTIFIER ||
            strcmp(fn->token->lexeme, "foo") != 0)
            ok = 0;
    }

    if (ok && call->child_count >= 2) {
        ASTNode *arg = child(call, 1);
        if (!arg || arg->type != AST_EXPR ||
            !arg->token || arg->token->type != TOK_PLUS)
            ok = 0;
    }

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 4) Program:
 *  if (1 + 2 * 3) { ... }
 */
static int test_parser_if_condition_expr(void)
{
    const char *name = "parser_if_condition_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1 + 2 * 3) {\n"
        "            return 1;\n"
        "        } else {\n"
        "            return 2;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected =
        "IF cond -> EXPR '+', right=EXPR '*'";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode)
        ok = 0;

    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token || cond->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(cond, 0) : NULL;
    ASTNode *R = ok ? child(cond, 1) : NULL;

    if (!L || !R)
        ok = 0;
    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;
    if (ok && (R->type != AST_EXPR || !R->token || R->token->type != TOK_STAR))
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 5) Program:
 *  while (1 + 2 * 3) { return 1; }
 */
static int test_parser_while_condition_expr(void)
{
    const char *name = "parser_while_condition_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        while (1 + 2 * 3) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected =
        "WHILE cond -> EXPR '+', right=EXPR '*'";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *wh = find_first(root, AST_WHILE);
    if (!wh)
        ok = 0;

    ASTNode *cond = ok ? child(wh, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token || cond->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(cond, 0) : NULL;
    ASTNode *R = ok ? child(cond, 1) : NULL;

    if (!L || !R)
        ok = 0;
    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;
    if (ok && (R->type != AST_EXPR || !R->token || R->token->type != TOK_STAR))
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 6) Program:
 *  return (1 + (2 * 3));
 */
static int test_parser_return_nested_parens(void)
{
    const char *name = "parser_return_nested_parens";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return (1 + (2 * 3));\n"
        "    }\n"
        "}\n";
    const char *expected =
        "RETURN -> EXPR '+', nested parens still give '+' root";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret)
        ok = 0;

    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token || expr->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(expr, 0) : NULL;
    ASTNode *R = ok ? child(expr, 1) : NULL;

    if (!L || !R)
        ok = 0;
    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;
    if (ok && (R->type != AST_EXPR || !R->token || R->token->type != TOK_STAR))
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 7) Program:
 *  if (1 + 2 * 3 == 7) { ... }
 */
static int test_parser_if_rel_eq_expr(void)
{
    const char *name = "parser_if_rel_eq_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1 + 2 * 3 == 7) {\n"
        "            return 1;\n"
        "        } else {\n"
        "            return 2;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected =
        "IF cond -> EXPR '==', left '+', right literal 7";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode)
        ok = 0;

    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token || cond->token->type != TOK_EQ)
        ok = 0;

    ASTNode *L = ok ? child(cond, 0) : NULL;
    ASTNode *R = ok ? child(cond, 1) : NULL;

    if (!L || !R)
        ok = 0;

    if (ok) {
        if (L->type != AST_EXPR || !L->token || L->token->type != TOK_PLUS)
            ok = 0;
        if (R->type != AST_LITERAL || !R->token || R->token->type != TOK_INT ||
            strcmp(R->token->lexeme, "7") != 0)
            ok = 0;
    }

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 8) Program:
 *  return x is Int;
 *
 * (typ 'Int' berieme ako IDENT)
 */
static int test_parser_return_is_operator(void)
{
    const char *name = "parser_return_is_operator";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return x is Int;\n"
        "    }\n"
        "}\n";
    const char *expected =
        "RETURN -> EXPR 'is' ...";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret)
        ok = 0;

    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token ||
        !expr->token->lexeme || strcmp(expr->token->lexeme, "is") != 0)
        ok = 0;

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 9) Program:
 *  return 1 + foo(2 * 3);
 */
static int test_parser_return_expr_with_call(void)
{
    const char *name = "parser_return_expr_with_call";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return 1 + foo(2 * 3);\n"
        "    }\n"
        "}\n";
    const char *expected =
        "RETURN -> EXPR '+', right contains CALL foo with arg 2*3";

    print_test_header(name, src, expected);

    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }

    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret)
        ok = 0;

    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token || expr->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(expr, 0) : NULL;
    ASTNode *R = ok ? child(expr, 1) : NULL;
    if (!L || !R)
        ok = 0;

    if (ok && (L->type != AST_LITERAL || !L->token || L->token->type != TOK_INT ||
               strcmp(L->token->lexeme, "1") != 0))
        ok = 0;

    ASTNode *call = NULL;
    if (ok && R->type == AST_CALL) {
        call = R;
    } else if (ok && R->type == AST_EXPR) {
        for (int i = 0; i < R->child_count; ++i) {
            if (R->children[i]->type == AST_CALL) {
                call = R->children[i];
                break;
            }
        }
    }

    if (!call)
        ok = 0;

    if (ok) {
        ASTNode *fn = child(call, 0);
        if (!fn || fn->type != AST_IDENTIFIER ||
            !fn->token || fn->token->type != TOK_IDENTIFIER ||
            strcmp(fn->token->lexeme, "foo") != 0)
            ok = 0;
    }

    if (ok && call->child_count >= 2) {
        ASTNode *arg = child(call, 1);
        if (!arg || arg->type != AST_EXPR ||
            !arg->token || arg->token->type != TOK_STAR)
            ok = 0;
    }

    print_result_line(ok, NULL);

    ast_free(root);
    return ok;
}

/*
 * 10) var a = 42;
 */
static int test_parser_var_decl_simple_int(void)
{
    const char *name = "parser_var_decl_simple_int";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var a = 42;\n"
        "    }\n"
        "}\n";
    const char *expected = "VAR_DECL a = 42";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *var = find_first(root, AST_VAR_DECL);
    if (!var) ok = 0;
    if (ok && (!var->token || !var->token->lexeme || strcmp(var->token->lexeme, "a") != 0))
        ok = 0;

    ASTNode *assign = ok ? child(var, 0) : NULL;
    if (!assign || assign->type != AST_ASSIGN) ok = 0;
    ASTNode *lit = ok ? child(assign, 0) : NULL;
    if (!lit || lit->type != AST_LITERAL || !lit->token ||
        lit->token->type != TOK_INT || strcmp(lit->token->lexeme, "42") != 0)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 11) var s = "a" + "b";
 */
static int test_parser_var_decl_string_concat(void)
{
    const char *name = "parser_var_decl_string_concat";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var s = \"a\" + \"b\";\n"
        "    }\n"
        "}\n";
    const char *expected = "VAR_DECL s = EXPR '+', literals \"a\" and \"b\"";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *var = find_first(root, AST_VAR_DECL);
    if (!var) ok = 0;

    ASTNode *assign = ok ? child(var, 0) : NULL;
    ASTNode *expr = assign ? child(assign, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token || expr->token->type != TOK_PLUS)
        ok = 0;

    ASTNode *L = ok ? child(expr, 0) : NULL;
    ASTNode *R = ok ? child(expr, 1) : NULL;
    if (!L || !R) ok = 0;
    if (ok && (!L->token || L->token->type != TOK_STRING)) ok = 0;
    if (ok && (!R->token || R->token->type != TOK_STRING)) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 12) return (1);
 */
static int test_parser_return_paren_only(void)
{
    const char *name = "parser_return_paren_only";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return (1);\n"
        "    }\n"
        "}\n";
    const char *expected = "RETURN -> LITERAL 1 (paren ok)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;
    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_LITERAL || !expr->token ||
        expr->token->type != TOK_INT || strcmp(expr->token->lexeme, "1") != 0)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 13) return foo();
 */
static int test_parser_return_fun_call_no_args(void)
{
    const char *name = "parser_return_fun_call_no_args";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return foo();\n"
        "    }\n"
        "}\n";
    const char *expected = "RETURN -> CALL foo() (no args)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;
    ASTNode *call = ok ? child(ret, 0) : NULL;
    if (!call || call->type != AST_CALL) ok = 0;

    ASTNode *fn = ok ? child(call, 0) : NULL;
    if (!fn || fn->type != AST_IDENTIFIER || !fn->token || !fn->token->lexeme ||
        strcmp(fn->token->lexeme, "foo") != 0)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 14) if (1) { return 1; }
 */
static int test_parser_if_literal_condition(void)
{
    const char *name = "parser_if_literal_condition";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> LITERAL 1";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_LITERAL || !cond->token ||
        cond->token->type != TOK_INT || strcmp(cond->token->lexeme, "1") != 0)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 15) if (foo()) { ... }
 */
static int test_parser_if_fun_call_condition(void)
{
    const char *name = "parser_if_fun_call_condition";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (foo()) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> CALL foo()";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_CALL) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 16) while (foo()) { return 1; }
 */
static int test_parser_while_fun_call_condition(void)
{
    const char *name = "parser_while_fun_call_condition";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        while (foo()) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "WHILE cond -> CALL foo()";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *wh = find_first(root, AST_WHILE);
    if (!wh) ok = 0;
    ASTNode *cond = ok ? child(wh, 0) : NULL;
    if (!cond || cond->type != AST_CALL) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 17) dve deklarácie:
 *    var a = 1;
 *    var b = 2;
 */
static int test_parser_two_var_decls(void)
{
    const char *name = "parser_two_var_decls";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var a = 1;\n"
        "        var b = 2;\n"
        "    }\n"
        "}\n";
    const char *expected = "BLOCK contains two VAR_DECL (a,b)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    int var_count = 0;

    if (find_first(root, AST_VAR_DECL)) {
        var_count = 1;
    } else {
        ok = 0;
    }

    ok = ok && (var_count >= 1);

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 18) return 1; return 2; v rôznych vetvách if/else
 */
static int test_parser_two_returns_in_if_else(void)
{
    const char *name = "parser_two_returns_in_if_else";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1) {\n"
        "            return 1;\n"
        "        } else {\n"
        "            return 2;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF + ELSE, oba obsahujú RETURN";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;

    ASTNode *else_node = find_first(root, AST_ELSE);
    if (!else_node) ok = 0;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 19) return foo() + bar(1);
 */
static int test_parser_chained_calls_in_expr(void)
{
    const char *name = "parser_chained_calls_in_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return foo() + bar(1);\n"
        "    }\n"
        "}\n";
    const char *expected = "RETURN -> EXPR '+', obsahuje CALL foo() a CALL bar(1)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;

    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;
    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token ||
        expr->token->type != TOK_PLUS)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 20) var a = foo(1+2);
 */
static int test_parser_call_in_var_init(void)
{
    const char *name = "parser_call_in_var_init";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var a = foo(1+2);\n"
        "    }\n"
        "}\n";
    const char *expected = "VAR_DECL a = CALL foo(1+2)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *var = find_first(root, AST_VAR_DECL);
    if (!var) ok = 0;
    ASTNode *assign = ok ? child(var, 0) : NULL;
    if (!assign || assign->type != AST_ASSIGN) ok = 0;
    ASTNode *call = ok ? child(assign, 0) : NULL;
    if (!call || call->type != AST_CALL) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 21) return 1 + 2 * 3 - 4 / 2;
 */
static int test_parser_complex_expr_mixed_ops(void)
{
    const char *name = "parser_complex_expr_mixed_ops";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return 1 + 2 * 3 - 4 / 2;\n"
        "    }\n"
        "}\n";
    const char *expected = "RETURN -> EXPR '-', obsahuje '+', '*', '/' podľa precedencie";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;
    ASTNode *expr = ok ? child(ret, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token ||
        expr->token->type != TOK_MINUS)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 22) if (1 < 2) { ... }
 */
static int test_parser_relational_less(void)
{
    const char *name = "parser_relational_less";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1 < 2) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> EXPR '<'";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token ||
        cond->token->type != TOK_LT)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 23) if (3 >= 2) { ... }
 */
static int test_parser_relational_ge(void)
{
    const char *name = "parser_relational_ge";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (3 >= 2) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> EXPR '>='";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token ||
        cond->token->type != TOK_GE)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 24) if (1 != 2) { ... }
 */
static int test_parser_equality_not_equal(void)
{
    const char *name = "parser_equality_not_equal";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1 != 2) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> EXPR '!='";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token ||
        cond->token->type != TOK_NE)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 25) if ((1 == 1) == (2 == 2)) { ... }
 *    – len uistíme sa, že koreň je '=='.
 */
static int test_parser_equality_chain_like(void)
{
    const char *name = "parser_equality_chain_like";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if ((1 == 1) == (2 == 2)) {\n"
        "            return 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF cond -> EXPR '==' (na koreni)";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ifnode = find_first(root, AST_IF);
    if (!ifnode) ok = 0;
    ASTNode *cond = ok ? child(ifnode, 0) : NULL;
    if (!cond || cond->type != AST_EXPR || !cond->token ||
        cond->token->type != TOK_EQ)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 26) vnorené if s výrazmi:
 *    if (1) {
 *      if (2+3) return 1;
 *    }
 */
static int test_parser_nested_if_with_exprs(void)
{
    const char *name = "parser_nested_if_with_exprs";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        if (1) {\n"
        "            if (2+3) {\n"
        "                return 1;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "IF v IF, vnútorný má EXPR '+'";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *expr_eq = find_first(root, AST_EXPR);
    if (!expr_eq) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 27) vnorený while:
 *    while (1) {
 *      while (2+3) return 1;
 *    }
 */
static int test_parser_nested_while_with_exprs(void)
{
    const char *name = "parser_nested_while_with_exprs";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        while (1) {\n"
        "            while (2+3) {\n"
        "                return 1;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    const char *expected = "WHILE v WHILE, vnútorný má EXPR '+'";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *wh = find_first(root, AST_WHILE);
    if (!wh) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 28) BLOCK s viacerými statementmi:
 *    var a = 1;
 *    return a;
 */
static int test_parser_block_multiple_statements(void)
{
    const char *name = "parser_block_multiple_statements";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var a = 1;\n"
        "        return a;\n"
        "    }\n"
        "}\n";
    const char *expected = "BLOCK obsahuje VAR_DECL a RETURN";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    if (!find_first(root, AST_VAR_DECL)) ok = 0;
    if (!find_first(root, AST_RETURN)) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 29) return (foo(1+2));
 */
static int test_parser_return_call_inside_parens(void)
{
    const char *name = "parser_return_call_inside_parens";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        return (foo(1+2));\n"
        "    }\n"
        "}\n";
    const char *expected = "RETURN -> CALL foo(1+2) cez PSA";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *ret = find_first(root, AST_RETURN);
    if (!ret) ok = 0;
    ASTNode *call = ok ? child(ret, 0) : NULL;
    if (!call || call->type != AST_CALL) ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/*
 * 30) var x = (1 + 2) * 3;
 */
static int test_parser_var_decl_with_paren_expr(void)
{
    const char *name = "parser_var_decl_with_paren_expr";
    const char *src =
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "    static main() {\n"
        "        var x = (1 + 2) * 3;\n"
        "    }\n"
        "}\n";
    const char *expected = "VAR_DECL x = EXPR '*', ľavý operand je EXPR '+'";

    print_test_header(name, src, expected);
    ASTNode *root = parse_program_from_string(src);
    if (!root) {
        print_result_line(0, "root=NULL");
        return 0;
    }
    printf("AST root:   %s\n", aststr(root->type));
    print_ast_tree(root);

    int ok = 1;
    ASTNode *var = find_first(root, AST_VAR_DECL);
    if (!var) ok = 0;
    ASTNode *assign = ok ? child(var, 0) : NULL;
    if (!assign || assign->type != AST_ASSIGN) ok = 0;
    ASTNode *expr = ok ? child(assign, 0) : NULL;
    if (!expr || expr->type != AST_EXPR || !expr->token ||
        expr->token->type != TOK_STAR)
        ok = 0;
    ASTNode *left = ok ? child(expr, 0) : NULL;
    if (!left || left->type != AST_EXPR || !left->token ||
        left->token->type != TOK_PLUS)
        ok = 0;

    print_result_line(ok, NULL);
    ast_free(root);
    return ok;
}

/* ---------------- main ---------------- */

int main(void)
{
    int total = 0;
    int passed = 0;

    printf("Running PARSER + PSA + AST integration tests...\n\n");

    RUN_TEST(test_parser_return_expr_1_plus_2_mul_3);
    RUN_TEST(test_parser_var_decl_expr);
    RUN_TEST(test_parser_return_fun_call_expr);
    RUN_TEST(test_parser_if_condition_expr);
    RUN_TEST(test_parser_while_condition_expr);
    RUN_TEST(test_parser_return_nested_parens);
    RUN_TEST(test_parser_if_rel_eq_expr);
    RUN_TEST(test_parser_return_is_operator);
    RUN_TEST(test_parser_return_expr_with_call);

    RUN_TEST(test_parser_var_decl_simple_int);
    RUN_TEST(test_parser_var_decl_string_concat);
    RUN_TEST(test_parser_return_paren_only);
    RUN_TEST(test_parser_return_fun_call_no_args);
    RUN_TEST(test_parser_if_literal_condition);
    RUN_TEST(test_parser_if_fun_call_condition);
    RUN_TEST(test_parser_while_fun_call_condition);
    RUN_TEST(test_parser_two_var_decls);
    RUN_TEST(test_parser_two_returns_in_if_else);
    RUN_TEST(test_parser_chained_calls_in_expr);
    RUN_TEST(test_parser_call_in_var_init);
    RUN_TEST(test_parser_complex_expr_mixed_ops);
    RUN_TEST(test_parser_relational_less);
    RUN_TEST(test_parser_relational_ge);
    RUN_TEST(test_parser_equality_not_equal);
    RUN_TEST(test_parser_equality_chain_like);
    RUN_TEST(test_parser_nested_if_with_exprs);
    RUN_TEST(test_parser_nested_while_with_exprs);
    RUN_TEST(test_parser_block_multiple_statements);
    RUN_TEST(test_parser_return_call_inside_parens);
    RUN_TEST(test_parser_var_decl_with_paren_expr);

    printf("Summary:    ");
    if (passed == total)
        printf(COLOR_GREEN "%d/%d tests PASSED" COLOR_RESET "\n", passed, total);
    else
        printf(COLOR_RED "%d/%d tests PASSED" COLOR_RESET "\n", passed, total);

    return (passed == total) ? 0 : 1;
}
