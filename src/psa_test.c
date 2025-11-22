#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "psa.h"

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

typedef struct {
    const char *name;
    const char *input;
    PsaResult   expected_result;
    TokenType   expected_end;
} TestCase;

static int run_one_test(const TestCase *tc)
{
    FILE *f = tmpfile();
    if (!f) return 0;

    fputs(tc->input, f);
    rewind(f);

    scanner_init(f);

    Token first = scanner_next();

    if (first.type == TOK_ERROR) {
        printf("[%-24s] FAIL (lexer error)\n", tc->name);
        fclose(f);
        return 0;
    }

    if (first.type == TOK_EOF) {
        printf("[%-24s] FAIL (empty expression)\n", tc->name);
        fclose(f);
        return 0;
    }

    Token end_tok;
    PsaResult res = psa_parse_expression(first, &end_tok, NULL);
    int pass = 1;

    if (res != tc->expected_result)
        pass = 0;

    if (tc->expected_end != TOK_ERROR && res == PSA_OK) {
        if (end_tok.type != tc->expected_end)
            pass = 0;
    }

    printf("[%-24s] %s\n", tc->name, pass ? "PASS" : "FAIL");
    printf("    input       : \"%s\"\n", tc->input);
    printf("    result      : %s (expected %s)\n",
           psa_result_name(res), psa_result_name(tc->expected_result));

    if (res == PSA_OK) {
        printf("    end token   : %s (expected %s)\n",
               token_type_name(end_tok.type),
               (tc->expected_end == TOK_ERROR)
                   ? "<ignored>"
                   : token_type_name(tc->expected_end));
    }
    printf("\n");

    fclose(f);
    return pass;
}

int main(void)
{
    const TestCase tests[] = {

        // ==========================================
        //  pôvodné CORRECT testy
        // ==========================================

        { "simple_num",             "1 + 2 * 3;",               PSA_OK, TOK_SEMICOLON },
        { "idents_mul",             "a + b * c;",               PSA_OK, TOK_SEMICOLON },
        { "paren_mul",              "(a + b) * c;",             PSA_OK, TOK_SEMICOLON },
        { "rel_eq",                 "a < b == c;",              PSA_OK, TOK_SEMICOLON },
        { "is_num",                 "a is Num;",                PSA_OK, TOK_SEMICOLON },
        { "eol_end",                "1 + 2 * 3\n",              PSA_OK, TOK_EOL },

        // ==========================================
        //  pôvodné ERROR testy
        // ==========================================

        { "missing_rhs",            "1 + ;",                    PSA_ERR_SYNTAX, TOK_ERROR },
        { "missing_rparen",         "(1 + 2;",                  PSA_ERR_SYNTAX, TOK_ERROR },
        { "bad_op_seq",             "1 < < 2;",                 PSA_ERR_SYNTAX, TOK_ERROR },

        // ==========================================
        // multiline základ
        // ==========================================

        { "multiline_plus",         "1 +\n2;",                  PSA_OK, TOK_SEMICOLON },
        { "multiline_plus_ws",      "1 +  \n   2;",             PSA_OK, TOK_SEMICOLON },
        { "multiline_end_nl",       "1 +\n2\n",                 PSA_OK, TOK_EOL },

        // ==========================================
        // náročnejšie správne výrazy
        // ==========================================

        { "nested_parens_1",        "((a + 1) * (b - 2)) / 3;", PSA_OK, TOK_SEMICOLON },
        { "nested_parens_2",        "a + b * (c + d * (e - f));", PSA_OK, TOK_SEMICOLON },
        { "rel_is_eq_chain",        "a < b is Num == c != d;",   PSA_OK, TOK_SEMICOLON },
        { "long_op_chain",          "a + b * c - d / e + f * (g - h);", PSA_OK, TOK_SEMICOLON },

        // ==========================================
        // multiline náročné
        // ==========================================

        { "multiline_parens_1",     "(\n  a + b\n) *\n(c - d);\n", PSA_OK, TOK_EOL },
        { "multiline_rel_is_eq",    "a <\n b is Num\n == c\n;",     PSA_OK, TOK_SEMICOLON },
        { "multiline_long_chain",   "a +\n b * c\n - d / (e +\n f);\n", PSA_OK, TOK_EOL },

        // ==========================================
        // chybové multiline
        // ==========================================

        { "missing_operand_multiline",
          "a +\n b *\n ;",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "unbalanced_parens_multiline",
          "(\n  a + (b * c\n);\n",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "bad_rel_chain_multiline",
          "a <\n < b;\n",
          PSA_ERR_SYNTAX, TOK_ERROR },

        // ==========================================
        //  ---- nové komplexné testy (navrhnuté tebou) ----
        // ==========================================

        { "arith_chain_1",
          "1 + 2 - 3 * 4 / 5;",
          PSA_OK, TOK_SEMICOLON },

        { "arith_hex_float",
          "0xFF + 1.5 * 2;",
          PSA_OK, TOK_SEMICOLON },

        { "arith_gid_and_id",
          "__glob * 10 + x;",
          PSA_OK, TOK_SEMICOLON },

        { "arith_string_ops",
          "\"abc\" * 3 + \"x\";",
          PSA_OK, TOK_SEMICOLON },

        { "arith_md_only",
          "a*b/c*d;",
          PSA_OK, TOK_SEMICOLON },

        { "paren_deep_1",
          "(((a + b))) * c;",
          PSA_OK, TOK_SEMICOLON },

        { "paren_nested_md",
          "(a + b) * (c / (d - e));",
          PSA_OK, TOK_SEMICOLON },

        { "paren_rel_1",
          "(a + b) < (c - d);",
          PSA_OK, TOK_SEMICOLON },

        { "paren_is_type",
          "(a < b) is Num;",
          PSA_OK, TOK_SEMICOLON },

        { "paren_eq_1",
          "(a + b) == (c * d);",
          PSA_OK, TOK_SEMICOLON },

        { "rel_chain_1",
          "a < b < c;",
          PSA_OK, TOK_SEMICOLON },

        { "rel_chain_2",
          "a <= b >= c;",
          PSA_OK, TOK_SEMICOLON },

        { "rel_eq_mix_1",
          "a == b < c;",
          PSA_OK, TOK_SEMICOLON },

        { "neq_is_mix",
          "a != b is Num;",
          PSA_OK, TOK_SEMICOLON },

        { "rel_eq_long_1",
          "a < b == c != d < e;",
          PSA_OK, TOK_SEMICOLON },

        { "ml_op_at_end_1",
          "a +\n b + c;\n",
          PSA_OK, TOK_EOL },

        { "ml_paren_inner_1",
          "(\n  a +\n  b\n) + c;\n",
          PSA_OK, TOK_EOL },

        { "ml_is_chain_1",
          "a is\n Num ==\n b;\n",
          PSA_OK, TOK_EOL },

        { "ml_mix_ops_1",
          "a + b\n - c *\n d;\n",
          PSA_OK, TOK_EOL },

        { "ml_paren_after_1",
          "(\n a + b\n) +\n c;\n",
          PSA_OK, TOK_EOL },

        // chybové rozšírenia
        { "err_op_at_start",
          "+ a * b;",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "err_double_op",
          "a + * b;",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "err_missing_rhs_paren2",
          "a + (b * );",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "err_lone_rparen",
          "a + b);",
          PSA_ERR_SYNTAX, TOK_ERROR },

        { "err_empty_paren",
          "();",
          PSA_ERR_SYNTAX, TOK_ERROR },
    };

    int count = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    for (int i = 0; i < count; ++i)
        passed += run_one_test(&tests[i]);

    printf("Summary: %d / %d tests passed.\n", passed, count);

    return (passed == count) ? 0 : 1;
}
