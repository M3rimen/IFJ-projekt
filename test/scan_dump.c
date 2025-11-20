#include <stdio.h>
#include "scanner.h"

static const char *tt(TokenType t)
{
    switch (t) {
    case TOK_IDENTIFIER: return "IDENT";
    case TOK_GID:        return "GID";
    case TOK_KEYWORD:    return "KEYWORD";
    case TOK_INT:        return "INT";
    case TOK_FLOAT:      return "FLOAT";
    case TOK_HEX:        return "HEX";
    case TOK_STRING:     return "STRING";

    case TOK_PLUS:       return "PLUS";
    case TOK_MINUS:      return "MINUS";
    case TOK_STAR:       return "STAR";
    case TOK_SLASH:      return "SLASH";
    case TOK_ASSIGN:     return "ASSIGN";
    case TOK_EQ:         return "EQ";
    case TOK_NE:         return "NE";
    case TOK_LT:         return "LT";
    case TOK_LE:         return "LE";
    case TOK_GT:         return "GT";
    case TOK_GE:         return "GE";

    case TOK_LPAREN:     return "LPAREN";
    case TOK_RPAREN:     return "RPAREN";
    case TOK_LBRACE:     return "LBRACE";
    case TOK_RBRACE:     return "RBRACE";
    case TOK_COMMA:      return "COMMA";
    case TOK_DOT:        return "DOT";
    case TOK_SEMICOLON:  return "SEMICOLON";
    case TOK_COLON:      return "COLON";
    case TOK_QUESTION:   return "QUESTION";

    case TOK_EOL:        return "EOL";
    case TOK_EOF:        return "EOF";
    case TOK_ERROR:      return "ERROR";

    default:             return "UNKNOWN";
    }
}

int main(void)
{
    // zmeň tu input podľa potreby
    const char *input =
        "(\n"
        "  a + b\n"
        ") *\n"
        "(c - d);\n";

    FILE *f = tmpfile();
    if (!f) {
        fprintf(stderr, "tmpfile failed\n");
        return 1;
    }

    fputs(input, f);
    rewind(f);

    scanner_init(f);

    while (1) {
        Token t = scanner_next();
        printf("%s", tt(t.type));
        if (t.lexeme) {
            printf(" [\"%s\"]", t.lexeme);
        }
        printf("\n");

        if (t.type == TOK_EOF || t.type == TOK_ERROR)
            break;
    }

    return 0;
}
