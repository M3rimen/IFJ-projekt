#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "psa.h"
#include "ast.h"

// ------------------------------------------------------------
// Token → text
// ------------------------------------------------------------

static const char *token_type_name(TokenType t)
{
    switch (t) {
    case TOK_IDENTIFIER:  return "IDENT";
    case TOK_GID:         return "GID";
    case TOK_KEYWORD:     return "KEYWORD";
    case TOK_INT:         return "INT";
    case TOK_FLOAT:       return "FLOAT";
    case TOK_HEX:         return "HEX";
    case TOK_STRING:      return "STRING";
    case TOK_PLUS:        return "PLUS";
    case TOK_MINUS:       return "MINUS";
    case TOK_STAR:        return "STAR";
    case TOK_SLASH:       return "SLASH";
    case TOK_ASSIGN:      return "ASSIGN";
    case TOK_EQ:          return "EQ";
    case TOK_LT:          return "LT";
    case TOK_LE:          return "LE";
    case TOK_GT:          return "GT";
    case TOK_GE:          return "GE";
    case TOK_NE:          return "NE";
    case TOK_LPAREN:      return "LPAREN";
    case TOK_RPAREN:      return "RPAREN";
    case TOK_LBRACE:      return "LBRACE";
    case TOK_RBRACE:      return "RBRACE";
    case TOK_COMMA:       return "COMMA";
    case TOK_DOT:         return "DOT";
    case TOK_SEMICOLON:   return "SEMICOLON";
    case TOK_COLON:       return "COLON";
    case TOK_QUESTION:    return "QUESTION";
    case TOK_EOF:         return "EOF";
    case TOK_EOL:         return "EOL";
    case TOK_ERROR:       return "ERROR";
    default:              return "UNKNOWN";
    }
}

static const char *psa_result_name(PsaResult r)
{
    switch (r) {
    case PSA_OK:           return "PSA_OK";
    case PSA_ERR_SYNTAX:   return "PSA_ERR_SYNTAX";
    case PSA_ERR_INTERNAL: return "PSA_ERR_INTERNAL";
    default:               return "PSA_???";
    }
}

// ------------------------------------------------------------
// AST typ → text
// ------------------------------------------------------------

static const char *ast_type_name(AST_TYPE t)
{
    switch (t) {
    case AST_PROGRAM:    return "PROGRAM";
    case AST_CLASS:      return "CLASS";
    case AST_FUNCTION:   return "FUNCTION";
    case AST_GETTER:     return "GETTER";
    case AST_SETTER:     return "SETTER";

    case AST_PARAM_LIST: return "PARAM_LIST";
    case AST_BLOCK:      return "BLOCK";
    case AST_STATEMENTS: return "STATEMENTS";

    case AST_VAR_DECL:   return "VAR_DECL";
    case AST_ASSIGN:     return "ASSIGN";
    case AST_CALL:       return "CALL";
    case AST_RETURN:     return "RETURN";
    case AST_IF:         return "IF";
    case AST_WHILE:      return "WHILE";

    case AST_EXPR:       return "EXPR";
    case AST_IDENTIFIER: return "IDENT";
    case AST_LITERAL:    return "LITERAL";

    default:             return "<UNKNOWN>";
    }
}

// ------------------------------------------------------------
// Pomocný buffer (pre S-expr)
// ------------------------------------------------------------

static void buf_append(char **buf, size_t *len, size_t *cap, const char *s)
{
    size_t slen = strlen(s);

    if (*len + slen + 1 > *cap) {
        size_t newcap = (*cap == 0 ? 64 : *cap * 2);
        while (newcap < *len + slen + 1)
            newcap *= 2;

        char *tmp = realloc(*buf, newcap);
        if (!tmp) return;

        *buf = tmp;
        *cap = newcap;
    }

    memcpy(*buf + *len, s, slen);
    *len += slen;
    (*buf)[*len] = '\0';
}

// ------------------------------------------------------------
// AST → S-expression (pre binárne EXPR uzly)
// ------------------------------------------------------------

static const char *op_string_from_token(const Token *t)
{
    if (!t) return "?";

    if (t->lexeme && t->lexeme[0] != '\0')
        return t->lexeme;

    switch (t->type) {
    case TOK_PLUS:   return "+";
    case TOK_MINUS:  return "-";
    case TOK_STAR:   return "*";
    case TOK_SLASH:  return "/";
    case TOK_LT:     return "<";
    case TOK_LE:     return "<=";
    case TOK_GT:     return ">";
    case TOK_GE:     return ">=";
    case TOK_EQ:     return "==";
    case TOK_NE:     return "!=";
    default:         return "?";
    }
}

static void ast_to_sexpr_rec(ASTNode *n, char **buf, size_t *len, size_t *cap)
{
    if (!n) {
        buf_append(buf, len, cap, "NULL");
        return;
    }

    switch (n->type) {

    case AST_LITERAL:
    case AST_IDENTIFIER:
        if (n->token && n->token->lexeme)
            buf_append(buf, len, cap, n->token->lexeme);
        else
            buf_append(buf, len, cap, "<val>");
        break;

    case AST_EXPR: {
        const char *op = op_string_from_token(n->token);

        buf_append(buf, len, cap, "(");
        buf_append(buf, len, cap, op);
        buf_append(buf, len, cap, " ");

        if (n->child_count >= 1)
            ast_to_sexpr_rec(n->children[0], buf, len, cap);
        else
            buf_append(buf, len, cap, "NULL");

        buf_append(buf, len, cap, " ");

        if (n->child_count >= 2)
            ast_to_sexpr_rec(n->children[1], buf, len, cap);
        else
            buf_append(buf, len, cap, "NULL");

        buf_append(buf, len, cap, ")");
        break;
    }

    default:
        buf_append(buf, len, cap, "<node>");
        break;
    }
}

static char *ast_to_sexpr(ASTNode *root)
{
    char *buf = NULL;
    size_t len = 0, cap = 0;

    ast_to_sexpr_rec(root, &buf, &len, &cap);

    if (!buf) {
        buf = malloc(1);
        buf[0] = '\0';
    }

    return buf;
}

// ------------------------------------------------------------
// AST
// ------------------------------------------------------------

void ast_print_pretty(ASTNode *n, const char *prefix, int is_last)
{
    if (!n) return;

    printf("%s", prefix);

    if (is_last)
        printf("└── ");
    else
        printf("├── ");

    printf("%s", ast_type_name(n->type));

    if (n->token && n->token->lexeme)
        printf(" [%s]", n->token->lexeme);

    printf("\n");

    char new_prefix[256];
    strcpy(new_prefix, prefix);

    if (is_last)
        strcat(new_prefix, "    ");
    else
        strcat(new_prefix, "│   ");

    for (int i = 0; i < n->child_count; ++i)
        ast_print_pretty(n->children[i], new_prefix, i == n->child_count - 1);
}

// ------------------------------------------------------------
// Test infraštruktúra
// ------------------------------------------------------------

typedef struct {
    const char *name;
    const char *input;
} AstPsaTest;

static int run_one_ast_test(const AstPsaTest *tc)
{
    FILE *f = tmpfile();
    if (!f) {
        printf("tmpfile() failed\n");
        return 0;
    }

    fputs(tc->input, f);
    rewind(f);

    scanner_init(f);
    Token first = scanner_next();

    if (first.type == TOK_ERROR) {
        printf("[%-20s] FAIL (lexer error)\n", tc->name);
        fclose(f);
        return 0;
    }

    if (first.type == TOK_EOF) {
        printf("[%-20s] FAIL (empty)\n", tc->name);
        fclose(f);
        return 0;
    }

    Token end_tok;
    ASTNode *root = NULL;

    PsaResult res = psa_parse_expression(first, &end_tok, &root);
    int pass = (res == PSA_OK && root != NULL);

    printf("[%-20s] %s\n", tc->name, pass ? "PASS" : "FAIL");
    printf("    input       : \"%s\"\n", tc->input);
    printf("    result      : %s\n", psa_result_name(res));

    if (res == PSA_OK) {
        printf("    end token   : %s\n", token_type_name(end_tok.type));

        char *sexpr = ast_to_sexpr(root);
        printf("    AST S-expr  : %s\n", sexpr);
        free(sexpr);

        printf("    AST STROM:\n");
        ast_print_pretty(root, "", 1);
    }

    printf("\n");

    ast_free(root);
    fclose(f);
    return pass;
}

int main(void)
{
    const AstPsaTest tests[] = {
        { "lit_simple",         "1;" },
        { "ident_simple",       "a;" },
        { "arith_precedence",   "1 + 2 * 3;" },
        { "arith_parens",       "(1 + 2) * 3;" },
        { "idents_mul",         "a + b * c;" },
        { "rel_eq",             "a < b == c;" },
        { "is_num",             "a is Num;" },
        { "rel_is_eq_chain",    "a < b is Num == c != d;" },
        { "multiline_plus",     "1 +\n2;" },
        { "multiline_long",     "a +\n b * c\n - d / (e +\n f);\n" },
        { "deep_parens",        "(((a + b))) * c;" },
    };

    int count = (int)(sizeof(tests) / sizeof(tests[0]));
    int passed = 0;

    for (int i = 0; i < count; ++i)
        passed += run_one_ast_test(&tests[i]);

    printf("AST+PSA visual test summary: %d / %d passed\n",
           passed, count);

    return 0;
}
