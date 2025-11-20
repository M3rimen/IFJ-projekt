#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "scanner.h"
#include "token.h"

#define INITIAL_BUF_SIZE 64

static FILE *input = NULL;
int current_char = ' ';

static char *cstrdup(const char *s);

// -------------------- Macro for repeated pattern --------------------
// Append character, advance, and change state
#define APPEND_ADVANCE_STATE(LEX, LEN, CAP, CH, NEXT_STATE) \
    do                                                      \
    {                                                       \
        buffer_append((LEX), (LEN), (CAP), (CH));           \
        advance();                                          \
        state = (NEXT_STATE);                               \
    } while (0)

// For single-character literals
#define RETURN_SINGLE_CHAR_TOKEN(TYPE) \
    do                                 \
    {                                  \
        advance();                     \
        free(lex);                     \
        return make_token(TYPE, NULL); \
    } while (0)

// -------------------- Utility Functions --------------------
static void advance() { current_char = fgetc(input); }
static int peek() { return current_char; }

void scanner_init(FILE *in)
{
    input = in;
    current_char = ' ';
    advance();
}

static Token make_token(TokenType type, char *lexeme)
{
    Token t;
    t.type = type;
    t.lexeme = lexeme;
    return t;
}

static Token make_error(const char *msg)
{
    advance();
    return make_token(TOK_ERROR, cstrdup(msg));
}

static void buffer_append(char **buf, size_t *len, size_t *cap, char c)
{
    if (*len + 1 >= *cap)
    {
        *cap *= 2;
        char *tmp = realloc(*buf, *cap);
        if (!tmp)
        {
            fprintf(stderr, "Fatal: Out of memory in lexer\n");
            free(*buf);
            exit(1);
        }
        *buf = tmp;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

static char *cstrdup(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p)
        memcpy(p, s, len);
    return p;
}

// -------------------- Whitespace & Comment Handling --------------------
static bool skip_block_comment()
{
    int depth = 1;
    while (peek() != EOF)
    {
        int c = peek();
        advance();
        if (c == '/' && peek() == '*')
        {
            advance();
            depth++;
            continue;
        }
        if (c == '*' && peek() == '/')
        {
            advance();
            depth--;
            if (depth == 0)
                return true;
        }
    }
    return false; // unterminated comment
}

static Token skip_whitespace()
{
    while (1)
    {
        int p = peek();
        if (p == ' ' || p == '\t' || p == '\r')
        {
            advance();
            continue;
        }
        else if (p == '\n')
        {
            advance();
            return make_token(TOK_EOL, NULL);
        }
        else if (p == '/')
        {
            advance();
            if (peek() == '/')
            {
                while (peek() != '\n' && peek() != EOF)
                    advance();

                if (peek() == '\n')
                    advance();

                return make_token(TOK_EOL, NULL);
            }
            else if (peek() == '*')
            {
                advance();
                bool closed = skip_block_comment();
                if (!closed)
                    return make_error("Unterminated block comment");
                continue;
            }
            else
            {
                current_char = '/'; // put back the '/'
                return make_token(TOK_WS, NULL);
            }
        }
        else
            return make_token(TOK_WS, NULL);
    }
}

// -------------------- Keyword checking --------------------
static bool is_keyword(const char *lex)
{
    static const char *keywords[] = {
        "class", "if", "else", "is", "null", "return", "var", "while", "static", "import", "for",
        "Num", "String", "Null", "Ifj"};
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
        if (strcmp(lex, keywords[i]) == 0)
            return true;
    return false;
}

// -------------------- Main Scanner --------------------
Token scanner_next()
{
    LexerState state = STATE_START;

    size_t len = 0, cap = INITIAL_BUF_SIZE;
    char *lex = malloc(cap);
    if (!lex)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    lex[0] = '\0';

    while (1)
    {
        switch (state)
        {
        case STATE_START:
        {
            Token tmp_token;
            tmp_token = skip_whitespace();

            if (tmp_token.type == TOK_EOL)
            {
                free(lex);
                return tmp_token;
            }

            if (tmp_token.type == TOK_ERROR)
            {
                free(lex);
                return tmp_token;
            }

            if (peek() == EOF)
                RETURN_SINGLE_CHAR_TOKEN(TOK_EOF);
            if (peek() == '0')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '0', STATE_SINGLE_ZERO);
                break;
            }
            else if (isdigit(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_INT);
                break;
            }
            else if (isalpha(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_ID);
                break;
            }
            else if (peek() == '"')
            {
                advance();
                state = STATE_PRE_STRING;
                break;
            }
            else if (peek() == '_')
            {
                advance();
                if (peek() == '_')
                {
                    buffer_append(&lex, &len, &cap, '_');
                    APPEND_ADVANCE_STATE(&lex, &len, &cap, '_', STATE_PRE_GID);
                    break;
                }
                else
                {
                    free(lex);
                    return make_error("Identifiers cannot start with single '_'");
                }
            }

            // -------------------- Operators & punctuation --------------------
            switch (peek())
            {
            case '+':
                RETURN_SINGLE_CHAR_TOKEN(TOK_PLUS);
            case '-':
                RETURN_SINGLE_CHAR_TOKEN(TOK_MINUS);
            case '*':
                RETURN_SINGLE_CHAR_TOKEN(TOK_STAR);
            case '/':
                RETURN_SINGLE_CHAR_TOKEN(TOK_SLASH);
            case '=':
                advance();
                if (peek() == '=')
                    RETURN_SINGLE_CHAR_TOKEN(TOK_EQ);
                RETURN_SINGLE_CHAR_TOKEN(TOK_ASSIGN);

            case '!':
                advance();
                if (peek() == '=')
                    RETURN_SINGLE_CHAR_TOKEN(TOK_NE);
                free(lex);
                return make_error("Unexpected '!': did you mean '!=' ?");

            case '<':
                advance();
                if (peek() == '=')
                    RETURN_SINGLE_CHAR_TOKEN(TOK_LE);
                RETURN_SINGLE_CHAR_TOKEN(TOK_LT);

            case '>':
                advance();
                if (peek() == '=')
                    RETURN_SINGLE_CHAR_TOKEN(TOK_GE);
                RETURN_SINGLE_CHAR_TOKEN(TOK_GT);

            case '(':
                RETURN_SINGLE_CHAR_TOKEN(TOK_LPAREN);
            case ')':
                RETURN_SINGLE_CHAR_TOKEN(TOK_RPAREN);
            case '{':
                RETURN_SINGLE_CHAR_TOKEN(TOK_LBRACE);
            case '}':
                RETURN_SINGLE_CHAR_TOKEN(TOK_RBRACE);
            case ',':
                RETURN_SINGLE_CHAR_TOKEN(TOK_COMMA);
            case '.':
                RETURN_SINGLE_CHAR_TOKEN(TOK_DOT);
            case ';':
                RETURN_SINGLE_CHAR_TOKEN(TOK_SEMICOLON);
            case ':':
                RETURN_SINGLE_CHAR_TOKEN(TOK_COLON);
            case '?':
                RETURN_SINGLE_CHAR_TOKEN(TOK_QUESTION);

            default:
                free(lex);
                return make_error("Unexpected character");
            }
        }
        case STATE_PRE_GID:
            if (isalnum(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_GID);
                break;
            }
            free(lex);
            return make_error("Invalid character after \"__\" ");

        case STATE_GID:
            while (isalnum(peek()) || peek() == '_')
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            return make_token(TOK_GID, lex); // ownership of lex passed

        case STATE_ID:
            while (isalnum(peek()) || peek() == '_')
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            if (is_keyword(lex))
                return make_token(TOK_KEYWORD, lex);
            return make_token(TOK_IDENTIFIER, lex);

        case STATE_SINGLE_ZERO:
            if (peek() == 'x' || peek() == 'X')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_HEX);
                break;
            }
            if (peek() == 'e' || peek() == 'E')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);
                break;
            }
            if (peek() == '.')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_FLOAT);
                break;
            }
            return make_token(TOK_INT, lex);

        case STATE_PRE_HEX:
            if (isxdigit(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_HEX);
                break;
            }
            free(lex);
            return make_error("Invalid hexadecimal int format");

        case STATE_HEX:
            while (isxdigit(peek()))
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            return make_token(TOK_HEX, lex);

        case STATE_PRE_FLOAT:
            if (isdigit(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_FLOAT);
                break;
            }
            free(lex);
            return make_error("Invalid decimal format");

        case STATE_FLOAT:
            while (isdigit(peek()))
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            if (peek() == 'e' || peek() == 'E')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);
                break;
            }
            return make_token(TOK_FLOAT, lex);

        case STATE_PRE_EXP:
            if (peek() == '+' || peek() == '-')
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            if (isdigit(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_EXP);
                break;
            }
            free(lex);
            return make_error("Invalid exponential format");

        case STATE_EXP:
            while (isdigit(peek()))
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            return make_token(TOK_FLOAT, lex);

        case STATE_INT:
            if (peek() == '.')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_FLOAT);
                break;
            }
            if (peek() == 'e' || peek() == 'E')
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);
                break;
            }
            if (isdigit(peek()))
            {
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_INT);
                break;
            }
            return make_token(TOK_INT, lex);

        case STATE_PRE_STRING:
            // check for triple quotes -> multiline string
            if (peek() == '"')
            {
                advance(); // second "
                if (peek() == '"')
                {
                    advance(); // third "
                    state = STATE_MULTIL_STRING;
                    break;
                }
                else
                {
                    // It was an empty string ""
                    free(lex);
                    return make_token(TOK_STRING, cstrdup(""));
                }
            }
            // normal single-line string
            state = STATE_IN_STRING;
            break;

        case STATE_IN_STRING:
            if (peek() == '\n' || peek() == EOF)
            {
                free(lex);
                return make_error("Unterminated string literal");
            }
            if (peek() == '\\')
            {
                advance();
                state = STATE_ESC;
                break;
            }
            if (peek() == '"')
            {
                advance();
                return make_token(TOK_STRING, lex);
            }
            if (peek() > 31)
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
                break;
            }
            free(lex);
            return make_error("Invalid control character in string");

        case STATE_ESC:
            if (peek() == EOF)
            {
                free(lex);
                return make_error("Unterminated escape sequence");
            }
            switch (peek())
            {
            case 'n':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\n', STATE_IN_STRING);
                break;
            case 'r':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\r', STATE_IN_STRING);
                break;
            case 't':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\t', STATE_IN_STRING);
                break;
            case '\\':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\\', STATE_IN_STRING);
                break;
            case '"':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '"', STATE_IN_STRING);
                break;
            case 'x':
            {
                advance(); // consume 'x'
                int h1 = peek();
                advance();
                int h2 = peek();
                advance();
                if (!isxdigit(h1) || !isxdigit(h2))
                {
                    free(lex);
                    return make_error("Invalid hex escape \\x??");
                }
                char hexbuf[3] = {(char)h1, (char)h2, 0};
                unsigned value = 0;
                sscanf(hexbuf, "%x", &value);
                buffer_append(&lex, &len, &cap, (char)value);
                state = STATE_IN_STRING;
                break;
            }
            default:
                free(lex);
                return make_error("Invalid escape sequence in string");
            }
            break;
        case STATE_MULTIL_STRING:
        {
            int line_start = 0;

            bool is_first_line = true;
            while (peek() != EOF)
            {
                /* ---------------------------------------------
                   Detect closing triple quotes
                   --------------------------------------------- */
                if (peek() == '"')
                {
                    advance();
                    if (peek() == '"')
                    {
                        advance();
                        if (peek() == '"')
                        {
                            // consume closing """
                            advance();
                            if (is_first_line)
                            {
                                // trim the first line (it is never trimmed in single-line mode)
                                // detect final content correctly

                                // detect if all chars are whitespace
                                bool only_ws = true;
                                for (size_t i = 0; i < len; i++)
                                {
                                    if (!isspace(lex[i]))
                                    {
                                        only_ws = false;
                                        break;
                                    }
                                }

                                if (only_ws)
                                {
                                    free(lex);
                                    return make_token(TOK_STRING, cstrdup(""));
                                }
                                else
                                {
                                    return make_token(TOK_STRING, lex);
                                }
                            }

                            /* -----------------------------------------
                                 — Trim FINAL LINE if blank
                               ----------------------------------------- */

                            bool final_blank = true;
                            for (size_t i = line_start; i < len; i++)
                                if (!isspace(lex[i]))
                                    final_blank = false;

                            if (final_blank)
                            {
                                len = line_start; // remove that line completely
                                lex[len] = '\0';
                            }

                            // Finally trim the newline before closing """
                            if (len > 0 && lex[len - 1] == '\n')
                            {
                                len--;
                                lex[len] = '\0';
                            }

                            return make_token(TOK_STRING, lex);
                        }
                        // not closing: write `""`
                        buffer_append(&lex, &len, &cap, '"');
                        buffer_append(&lex, &len, &cap, '"');
                        continue;
                    }
                    // not closing: write `"`
                    buffer_append(&lex, &len, &cap, '"');
                    continue;
                }

                /* ---------------------------------------------
                   Handle newline → start new line
                   --------------------------------------------- */
                if (peek() == '\n')
                {
                    advance();
                    buffer_append(&lex, &len, &cap, '\n');
                    if (is_first_line)
                    {
                        // trim the first line (it is never trimmed in single-line mode)
                        // detect final content correctly

                        // detect if all chars are whitespace
                        bool only_ws = true;
                        for (size_t i = 0; i < len; i++)
                        {
                            if (!isspace(lex[i]))
                            {
                                only_ws = false;
                                break;
                            }
                        }

                        if (only_ws)
                        {
                            while (len > 0)
                            {
                                len--;
                                lex[len] = '\0';
                            }
                        }
                    }
                    is_first_line = false;
                    line_start = len;
                    continue;
                }

                /* ---------------------------------------------
                   Any other character → append
                   --------------------------------------------- */
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            free(lex);
            return make_error("Unterminated multiline string literal");
        }
        } // end switch
    } // end while
} // end scanner_next
