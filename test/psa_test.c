#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "psa.h"

// -------------------- Helpers for printing --------------------

static const char *token_type_name(TokenType t)
{
    switch (t) {
    // identifiers & literals
    case TOK_IDENTIFIER:  return "IDENT";
    case TOK_GID:         return "GID";
    case TOK_KEYWORD:     return "KEYWORD";
    case TOK_INT:         return "INT";
    case TOK_FLOAT:       return "FLOAT";
    case TOK_HEX:         return "HEX";
    case TOK_STRING:      return "STRING";

    // operators
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

    // punctuation
    case TOK_LPAREN:      return "LPAREN";
    case TOK_RPAREN:      return "RPAREN";
    case TOK_LBRACE:      return "LBRACE";
    case TOK_RBRACE:      return "RBRACE";
    case TOK_COMMA:       return "COMMA";
    case TOK_DOT:         return "DOT";
    case TOK_SEMICOLON:   return "SEMICOLON";
    case TOK_COLON:       return "COLON";
    case TOK_QUESTION:    return "QUESTION";

    // special
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

// -------------------- Test infrastructure --------------------

typedef struct {
    const char *name;
    const char *input;
    PsaResult   expected_result;
    TokenType   expected_end;   // ak je TOK_ERROR => end token sa nekontroluje
} TestCase;

static int run_one_test(const TestCase *tc)
{
    FILE *f = tmpfile();
    if (!f) {
        fprintf(stderr, "Cannot create tmpfile()\n");
        return 0;
    }

    fputs(tc->input, f);
    rewind(f);

    scanner_init(f);

    Token first = scanner_next();

    if (first.type == TOK_ERROR) {
        printf("[%-18s] FAIL (lexer error: %s)\n",
               tc->name,
               first.lexeme ? first.lexeme : "unknown");
        fclose(f);
        return 0;
    }

    if (first.type == TOK_EOF) {
        printf("[%-18s] FAIL (empty input for expression)\n", tc->name);
        fclose(f);
        return 0;
    }

    Token end_tok;
    PsaResult res = psa_parse_expression(first, &end_tok);

    int pass = 1;

    if (res != tc->expected_result)
        pass = 0;

    if (tc->expected_end != TOK_ERROR && res == PSA_OK) {
        if (end_tok.type != tc->expected_end)
            pass = 0;
    }

    printf("[%-18s] %s\n", tc->name, pass ? "PASS" : "FAIL");
    printf("    input       : \"%s\"\n", tc->input);
    printf("    result      : %s (expected %s)\n",
           psa_result_name(res),
           psa_result_name(tc->expected_result));

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
        // -------------------- pôvodné jednoduché testy --------------------
        {
            "simple_num",
            "1 + 2 * 3;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "idents_mul",
            "a + b * c;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "paren_mul",
            "(a + b) * c;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "rel_eq",
            "a < b == c;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "is_num",
            "a is Num;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "eol_end",
            "1 + 2 * 3\n",
            PSA_OK,
            TOK_EOL
        },

        // chybové prípady
        {
            "missing_rhs",
            "1 + ;",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        },
        {
            "missing_rparen",
            "(1 + 2;",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        },
        {
            "bad_op_seq",
            "1 < < 2;",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        },

        // multiline testy – základ
        {
            "multiline_plus",
            "1 +\n2;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "multiline_plus_ws",
            "1 +  \n   2;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "multiline_end_nl",
            "1 +\n2\n",
            PSA_OK,
            TOK_EOL
        },

        // -------------------- náročnejšie korektné výrazy --------------------

        // viacnásobné zátvorky a operátory
        {
            "nested_parens_1",
            "((a + 1) * (b - 2)) / 3;",
            PSA_OK,
            TOK_SEMICOLON
        },
        {
            "nested_parens_2",
            "a + b * (c + d * (e - f));",
            PSA_OK,
            TOK_SEMICOLON
        },

        // kombinácia relatívnych, is a rovnostných
        {
            "rel_is_eq_chain",
            "a < b is Num == c != d;",
            PSA_OK,
            TOK_SEMICOLON
        },

        // viac operátorov v rade s precedenciou
        {
            "long_op_chain",
            "a + b * c - d / e + f * (g - h);",
            PSA_OK,
            TOK_SEMICOLON
        },

        // multiline s viacerými zátvorkami
        {
            "multiline_parens_1",
            "(\n  a + b\n) *\n(c - d);\n",
            PSA_OK,
            TOK_EOL
        },

        // multiline kombinácia rel + eq + is
        {
            "multiline_rel_is_eq",
            "a <\n b is Num\n == c\n;",
            PSA_OK,
            TOK_SEMICOLON
        },

        // reťazenie operátorov na viacerých riadkoch
        {
            "multiline_long_chain",
            "a +\n b * c\n - d / (e +\n f);\n",
            PSA_OK,
            TOK_EOL
        },

        // -------------------- náročnejšie chybové prípady --------------------

        // chýbajúci operand po viacerých riadkoch
        {
            "missing_operand_multiline",
            "a +\n b *\n ;",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        },

        // zlé zátvorky vo viacerých riadkoch
        {
            "unbalanced_parens_multiline",
            "(\n  a + (b * c\n);\n",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        },

        // dva relačné po sebe (malo by zostať chybou)
        {
            "bad_rel_chain_multiline",
            "a <\n < b;\n",
            PSA_ERR_SYNTAX,
            TOK_ERROR
        }
    };

    int count = (int)(sizeof(tests) / sizeof(tests[0]));
    int passed = 0;

    for (int i = 0; i < count; ++i)
        passed += run_one_test(&tests[i]);

    printf("Summary: %d / %d tests passed.\n", passed, count);

    return (passed == count) ? 0 : 1;
}
