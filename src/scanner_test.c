#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "token.h"

static const char *token_type_name(TokenType t)
{
    switch (t)
    {
    case TOK_IDENTIFIER:
        return "TOK_IDENTIFIER";
    case TOK_GID:
        return "TOK_GID";
    case TOK_KEYWORD:
        return "TOK_KEYWORD";
    case TOK_INT:
        return "TOK_INT";
    case TOK_FLOAT:
        return "TOK_FLOAT";
    case TOK_HEX:
        return "TOK_HEX";
    case TOK_STRING:
        return "TOK_STRING";

    case TOK_PLUS:
        return "TOK_PLUS";
    case TOK_MINUS:
        return "TOK_MINUS";
    case TOK_STAR:
        return "TOK_STAR";
    case TOK_SLASH:
        return "TOK_SLASH";
    case TOK_ASSIGN:
        return "TOK_ASSIGN";
    case TOK_EQ:
        return "TOK_EQ";
    case TOK_LT:
        return "TOK_LT";
    case TOK_LE:
        return "TOK_LE";
    case TOK_GT:
        return "TOK_GT";
    case TOK_GE:
        return "TOK_GE";
    case TOK_NE:
        return "TOK_NE";

    case TOK_LPAREN:
        return "TOK_LPAREN";
    case TOK_RPAREN:
        return "TOK_RPAREN";
    case TOK_LBRACE:
        return "TOK_LBRACE";
    case TOK_RBRACE:
        return "TOK_RBRACE";
    case TOK_COMMA:
        return "TOK_COMMA";
    case TOK_DOT:
        return "TOK_DOT";
    case TOK_SEMICOLON:
        return "TOK_SEMICOLON";
    case TOK_COLON:
        return "TOK_COLON";
    case TOK_QUESTION:
        return "TOK_QUESTION";

    case TOK_EOF:
        return "TOK_EOF";
    case TOK_EOL:
        return "TOK_EOL";
    case TOK_ERROR:
        return "TOK_ERROR";
    }
    return "TOK_UNKNOWN";
}

int main(void)
{
    FILE *in = fopen("test_input.txt", "r");
    FILE *out = fopen("tokens.out", "w");
    if (!out)
    {
        perror("tokens.out");
        return 1;
    }

    scanner_init(in);

    while (1)
    {
        Token tok = scanner_next();

        fprintf(out, "{%s, ", token_type_name(tok.type));

        if (tok.type == TOK_EOL)
        {
            if (tok.lexeme)
                free(tok.lexeme);
            fprintf(out, "}\n"); // blank line after EOL token
            continue;
        }

        if (tok.lexeme)
            fprintf(out, "\"%s\"}  ", tok.lexeme);
        else
            fprintf(out, "NULL}");

        if (tok.lexeme)
            free(tok.lexeme);
        if (tok.type == TOK_EOF)
            break;
    }

    printf("Done\n");
    fclose(in);
    fclose(out);
    return 0;
}

/*int main(void) {
    FILE *in = fopen("test_input.txt", "r");
    FILE *out = fopen("tokens.out", "w");
    if (!out) {
        perror("tokens.out");
        return 1;
    }

    scanner_init(in);

    while (1) {
        Token tok = scanner_next();

        if (tok.type == TOK_STRING) {
            fprintf(out, "{\"%s\"} \n", tok.lexeme);
        }

        if (tok.type == TOK_EOF)
            break;
        free(tok.lexeme);
    }

    printf("Done\n");
    fclose(out);
    return 0;
}*/