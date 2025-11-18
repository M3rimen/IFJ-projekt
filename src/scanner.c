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
    advance();
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

                if (peek() == '\n') 
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
                current_char = '/'; // put back the '/'
                return WS_NONE;
            }
        }
        else
            return WS_NONE;
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

// -------------------- Multiline String --------------------
static Token scan_multiline_string() {
    size_t cap = 64;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) exit(1);

    bool first_line = true;
    bool terminated = false;

    while (peek() != EOF) {
        // Check for closing triple quotes
        if (peek() == '"') {
            size_t quote_count = 0;
            while (peek() == '"' && quote_count < 3) {
                advance();
                quote_count++;
            }
            if (quote_count == 3) {
                terminated = true;
                break;
            } else {
                for (size_t i = 0; i < quote_count; i++)
                    buffer_append(&buf, &len, &cap, '"');
            }
        } else {
            char c = (char)peek();
            advance();

            if (first_line) {
                // Trim leading whitespace on the first line after opening """
                if (c == ' ' || c == '\t') continue;
                if (c == '\n') continue; // remove blank lines immediately after """
                first_line = false;
            }

            buffer_append(&buf, &len, &cap, c);
        }
    }

    if (!terminated) {
        free(buf);
        return make_error("Unterminated multiline string literal");
    }

    // Trim only the whitespace on the line of the closing """
    size_t end = len;
    while (end > 0 && (buf[end - 1] == ' ' || buf[end - 1] == '\t' || buf[end - 1] == '\n')) {
        // Stop trimming at the first non-newline character on previous lines
        if (buf[end - 1] == '\n') break;
        end--;
    }
    buf[end] = '\0';

    return make_token(TOK_STRING, buf);
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

            WSResult ws = skip_whitespace();
            if (ws == WS_EOL)
                return make_token(TOK_EOL, strdup("\n"));

            if (peek() == EOF)
            {
                free(lex);
                return make_token(TOK_EOF, strdup(""));
            }
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
                RETURN_SINGLE_CHAR_TOKEN('+', TOK_PLUS);
            case '-':
                RETURN_SINGLE_CHAR_TOKEN('-', TOK_MINUS);
            case '*':
                RETURN_SINGLE_CHAR_TOKEN('*', TOK_STAR);
            case '/':
                RETURN_SINGLE_CHAR_TOKEN('/', TOK_SLASH);
            case '=':
                advance();
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('=', '=', TOK_EQ);
                RETURN_SINGLE_CHAR_TOKEN('=', TOK_ASSIGN);

            case '!':
                advance();
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('!', '=', TOK_NE);
                free(lex);
                return make_error("Unexpected '!': did you mean '!=' ?");

            case '<':
                advance();
                if (peek() == '=')
                    RETURN_DOUBLE_CHAR_TOKEN('<', '=', TOK_LE);
                RETURN_SINGLE_CHAR_TOKEN('<', TOK_LT);

            case '>':
                advance();
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

        } // end switch
    } // end while
} // end scanner_next
