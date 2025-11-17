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

// -------------------- Macro for repeated pattern --------------------
// Append character, advance, and change state
#define APPEND_ADVANCE_STATE(LEX, LEN, CAP, CH, NEXT_STATE) \
    do                                                      \
    {                                                       \
        buffer_append((LEX), (LEN), (CAP), (CH));           \
        advance();                                          \
        state = (NEXT_STATE);                               \
        break;                                              \
    } while (0)

// For single-character literals
#define RETURN_SINGLE_CHAR_TOKEN(CH, TYPE) \
    do                                     \
    {                                      \
        advance();                         \
        lex[0] = (CH);                     \
        lex[1] = '\0';                     \
        return make_token(TYPE, lex);      \
    } while (0)

// For double-character literals (like ==, !=, <=, >=)
#define RETURN_DOUBLE_CHAR_TOKEN(CH1, CH2, TYPE) \
    do                                           \
    {                                            \
        advance();                               \
        advance();                               \
        lex[0] = (CH1);                          \
        lex[1] = (CH2);                          \
        lex[2] = '\0';                           \
        return make_token(TYPE, lex);            \
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
    return make_token(TOK_ERROR, msg ? strdup(msg) : NULL);
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
            exit(1);
        }
        *buf = tmp;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

// -------------------- Whitespace & Comment Handling --------------------
static void skip_block_comment()
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
                return;
        }
    }
    fprintf(stderr, "Unterminated block comment\n");
    exit(1);
}

static WSResult skip_whitespace()
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
            return WS_EOL;
        }
        else if (p == '/')
        {
            advance();
            if (peek() == '/')
            {
                while (peek() != '\n' && peek() != EOF)
                    advance();
                return WS_EOL;
            }
            else if (peek() == '*')
            {
                advance();
                skip_block_comment();
                continue;
            }
            else
            {
                ungetc('/', input);
                advance();
                return WS_NONE;
            }
        }
        else
            return WS_NONE;
    }
}

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

// -------------------- Multiline String --------------------
static Token scan_multiline_string()
{
    size_t cap = INITIAL_BUF_SIZE, len = 0;
    char *buf = malloc(cap);
    buf[0] = '\0';
    bool first_line = true;

    while (peek() != EOF)
    {
        if (peek() == '"')
        {
            int count = 0;
            while (peek() == '"' && count < 3)
            {
                advance();
                count++;
            }
            if (count == 3)
            {
                if (len > 0 && buf[len - 1] == '\n')
                    len--;
                return make_token(TOK_STRING, buf);
            }
            for (int i = 0; i < count; i++)
                buffer_append(&buf, &len, &cap, '"');
        }

        // Dynamic line reading
        size_t linecap = INITIAL_BUF_SIZE, linelen = 0;
        char *linebuf = malloc(linecap);
        if (!linebuf)
        {
            free(buf);
            exit(1);
        }
        while (peek() != '\n' && peek() != EOF)
        {
            if (linelen + 1 >= linecap)
            {
                linecap *= 2;
                linebuf = realloc(linebuf, linecap);
                if (!linebuf)
                {
                    free(buf);
                    exit(1);
                }
            }
            linebuf[linelen++] = (char)peek();
            advance();
        }
        linebuf[linelen] = '\0';
        if (peek() == '\n')
            advance();

        bool only_ws = true;
        for (size_t i = 0; i < linelen; i++)
            if (linebuf[i] != ' ' && linebuf[i] != '\t')
                only_ws = false;
        if (first_line && only_ws)
        {
            first_line = false;
            free(linebuf);
            continue;
        }
        first_line = false;
        if (only_ws)
        {
            free(linebuf);
            continue;
        }

        for (size_t i = 0; i < linelen; i++)
            buffer_append(&buf, &len, &cap, linebuf[i]);
        buffer_append(&buf, &len, &cap, '\n');
        free(linebuf);
    }

    free(buf);
    return make_error("Unterminated multiline string literal");
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
            WSResult ws = skip_whitespace();
            if (ws == WS_EOL)
                return make_token(TOK_EOL, strdup("\n"));

            if (peek() == '\n')
            {
                advance();
                return make_token(TOK_EOL, strdup("\n"));
            }
            if (peek() == EOF)
            {
                free(lex);
                return make_token(TOK_EOF, strdup(""));
            }
            if (peek() == '0')
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '0', STATE_SINGLE_ZERO);

            else if (isdigit(peek()))
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_INT);

            else if (isalpha(peek()))
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_ID);

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
                RETURN_SINGLE_CHAR_TOKEN('+', TOK_PLUS);
            case '-':
                RETURN_SINGLE_CHAR_TOKEN('-', TOK_MINUS);
            case '*':
                RETURN_SINGLE_CHAR_TOKEN('*', TOK_STAR);

            case '=':
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('=', '=', TOK_EQ);
                RETURN_SINGLE_CHAR_TOKEN('=', TOK_ASSIGN);

            case '!':
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('!', '=', TOK_NE);
                free(lex);
                return make_error("Unexpected '!': did you mean '!=' ?");

            case '<':
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('<', '=', TOK_LE);
                RETURN_SINGLE_CHAR_TOKEN('<', TOK_LT);

            case '>':
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('>', '=', TOK_GE);
                RETURN_SINGLE_CHAR_TOKEN('>', TOK_GT);

            case '(':
                RETURN_SINGLE_CHAR_TOKEN('(', TOK_LPAREN);
            case ')':
                RETURN_SINGLE_CHAR_TOKEN(')', TOK_RPAREN);
            case '{':
                RETURN_SINGLE_CHAR_TOKEN('{', TOK_LBRACE);
            case '}':
                RETURN_SINGLE_CHAR_TOKEN('}', TOK_RBRACE);
            case ',':
                RETURN_SINGLE_CHAR_TOKEN(',', TOK_COMMA);
            case '.':
                RETURN_SINGLE_CHAR_TOKEN('.', TOK_DOT);
            case ';':
                RETURN_SINGLE_CHAR_TOKEN(';', TOK_SEMICOLON);
            case ':':
                RETURN_SINGLE_CHAR_TOKEN(':', TOK_COLON);
            case '?':
                RETURN_SINGLE_CHAR_TOKEN('?', TOK_QUESTION);

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
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_HEX);

            if (peek() == 'e' || peek() == 'E')
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);

            if (peek() == '.')
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_FLOAT);

            return make_token(TOK_INT, lex);

        case STATE_PRE_HEX:
            if (isxdigit(peek()))
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_HEX);

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
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_FLOAT);

            free(lex);
            return make_error("Invalid decimal format");

        case STATE_FLOAT:
            while (isdigit(peek()))
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            if (peek() == 'e' || peek() == 'E')
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);

            return make_token(TOK_FLOAT, lex);

        case STATE_PRE_EXP:
            if (peek() == '+' || peek() == '-')
            {
                buffer_append(&lex, &len, &cap, (char)peek());
                advance();
            }
            if (isdigit(peek()))
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_EXP);

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
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_FLOAT);

            if (peek() == 'e' || peek() == 'E')
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_PRE_EXP);

            if (isdigit(peek()))
                APPEND_ADVANCE_STATE(&lex, &len, &cap, (char)peek(), STATE_INT);

            return make_token(TOK_INT, lex);

        case STATE_PRE_STRING:
            // check for triple quotes -> multiline string
            if (peek() == '"')
            {
                advance(); // second "
                if (peek() == '"')
                {
                    advance(); // third "
                    // begin multiline string (we consumed the opening """)
                    free(lex); // not used for multiline path
                    return scan_multiline_string();
                }
                else
                {
                    // It was an empty string ""
                    free(lex);
                    return make_token(TOK_STRING, strdup(""));
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
            case 'r':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\r', STATE_IN_STRING);
            case 't':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\t', STATE_IN_STRING);
            case '\\':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '\\', STATE_IN_STRING);
            case '"':
                APPEND_ADVANCE_STATE(&lex, &len, &cap, '"', STATE_IN_STRING);
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
            // We never use STATE_MULTIL_STRING in the new flow: multiline path returns directly.
            // Keep for completeness, but delegate to scan_multiline_string already.
            free(lex);
            return scan_multiline_string();
        } // end switch
    } // end while
} // end scanner_next
