// scanner
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "scanner.h"
#include "token.h"

#define INITIAL_BUF_SIZE 64

static FILE *input = NULL;
int current_char = ' ';

// Fetch next character from stream
static void advance()
{
    current_char = fgetc(input);
}

// Return current character without consuming it
static int peek()
{
    return current_char;
}

// Initialize scanner
void scanner_init(FILE *in)
{
    input = in;
    current_char = ' ';
    advance();
}

// Create a token helper
static Token make_token(TokenType type, const char *lexeme)
{
    Token t;
    t.type = type;
    t.lexeme = lexeme ? strdup(lexeme) : NULL;
    return t;
}

// Create error token
static Token make_error(const char *msg)
{
    return make_token(TOK_ERROR, msg);
}
// Helper function to append
static void buffer_append(char **buf, size_t *len, size_t *cap, char c)
{
    if (*len + 1 >= *cap)
    {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
        if (!*buf)
        {
            fprintf(stderr, "Fatal: Out of memory in lexer\n");
            exit(1);
        }
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}

// Helper function to skip multi line comments
static void skip_block_comment()
{
    int depth = 1; // we already saw one "/*" by the time this function is called

    while (peek() != EOF)
    {
        advance();

        // Check start of nested comment
        if (peek() == '/')
        {
            advance();
            if (peek() == '*')
            {
                depth++;
            }
            continue;
        }

        // Check end of comment
        if (peek() == '*')
        {
            advance();
            if (peek() == '/')
            {
                depth--;
                if (depth == 0)
                {
                    advance();
                    return; // end of entire comment
                }
            }
            continue;
        }
    }

    // If we got here, EOF before closed comment
    fprintf(stderr, "Unterminated block comment\n");
    exit(1); // lexical error 1 in your return-value convention
}

// Skip whitespace and comments
static WSResult skip_whitespace()
{
    while (1)
    {
        if (peek() == ' ' || peek() == '\t' || peek() == '\r')
        {
            advance();
        }

        else if (peek() == '\n')
        {
            advance();
            return WS_EOL; // newline counts as token delimiter
        }

        else if (peek() == '/')
        {
            advance();
            if (peek() == '/')
            {
                // line comment -> treat as newline
                while (peek() != '\n' && peek() != EOF)
                    advance();
                // do not consume the '\n' here
                return WS_EOL;
            }
            else if (peek() == '*')
            {
                skip_block_comment(); // counts as a single 'normal' WS not an EOL
            }
            else
            {
                // It was just '/', rewind to let scanner handle the operator
                ungetc('/', input);
                advance();
                return WS_NONE;
            }
        }

        else
        {
            return WS_NONE;
        }
    }
}

// Main scanner function
Token scanner_next()
{
    LexerState state = STATE_START;

    size_t len = 0, cap = INITIAL_BUF_SIZE;
    char *lex = malloc(cap);
    lex[0] = '\0';

    for (;;)
    {
        switch (state)
        {

        case STATE_START:
            WSResult ws = skip_whitespace();
            if (ws == WS_EOL)
                return make_token(TOK_EOL, "\n");

            if (peek() == '\n')
            { // EOL
                advance();
                return make_token(TOK_EOL, "\n");
            }
            else if (peek() == EOF)
            { // EOF
                return make_token(TOK_EOF, "");
            }

            else if (peek() == '0')
            { // 0
                buffer_append(&lex, &len, &cap, '0');
                advance();
                state = STATE_SINGLE_ZERO;
                break;
            }

            else if (isdigit(peek()))
            { // 1-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_INT;
                break;
            }

            else if (isalpha(peek()))
            { // a-Z
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_ID;
                break;
            }

            else if (peek() == '"')
            { // "
                advance();
                state = STATE_IN_STRING;
                break;
                ;
            }

            else if (peek() == '_')
            { // '_'
                advance();
                if (peek() == '_')
                { // "__"
                    buffer_append(&lex, &len, &cap, '_');
                    buffer_append(&lex, &len, &cap, '_');
                    advance();
                    state = STATE_PRE_GID;
                    break;
                }
                else
                    return make_error("Identifiers cannot start with single '_'");
            }
            else if (peek() == '(')
            { // TODO, finish operation/punctuation states
                advance();
                return make_token(TOK_LPAREN, "(");
            }
            return make_error("Unexpected character");

        case STATE_PRE_GID:
            if (isalnum(peek()))
            { // a-Z, 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_GID;
                break;
            }
            return make_error("Invalid character after \"__\" ");

        case STATE_GID:
            while (isalnum(peek()) || peek() == '_')
            { // a-Z, 0-9, '_'
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            return make_token(TOK_GID, lex);

        case STATE_ID:
            while (isalnum(peek()) || peek() == '_')
            { // a-Z, 0-9, '_'
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            /*TODO
                check KEYWORD
            TODO*/
            return make_token(TOK_IDENTIFIER, lex);

        case STATE_KEY:
            // TODO
        case STATE_SINGLE_ZERO:
            if (peek() == 'x' || peek() == 'X')
            { // 'x'/'X'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_HEX;
                break;
            }
            if (peek() == 'e' || peek() == 'E')
            { // 'e'/'E'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_EXP;
                break;
            }
            if (peek() == '.')
            { // '.'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_FLOAT;
                break;
            }
            return make_token(TOK_INT, lex);

        case STATE_PRE_HEX:
            if (isxdigit(peek()))
            { // HEX_DIGIT
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_HEX;
                break;
            }
            return make_error("Invalid hexadecimal int format");

        case STATE_HEX:
            while (isxdigit(peek()))
            { // HEX_DIGIT
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            return make_token(TOK_HEX, lex);

        case STATE_PRE_FLOAT:
            if (isdigit(peek()))
            { // 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_FLOAT;
                break;
            }
            return make_error("Invalid decimal format");

        case STATE_FLOAT:
            while (isdigit(peek()))
            { // 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            if (peek() == 'e' || peek() == 'E')
            { // 'e'/'E'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_EXP;
                break;
            }
            return make_token(TOK_FLOAT, lex);

        case STATE_PRE_EXP:
            if (peek() == '+' || peek() == '-')
            { // '+','-'
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            if (isdigit(peek()))
            { // 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_EXP;
                break;
            }
            return make_error("Invalid exponential format");

        case STATE_EXP:
            while (isdigit(peek()))
            { // 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
            }
            return make_token(TOK_FLOAT, lex);

        case STATE_INT:
            if (peek() == '.')
            { // '.'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_FLOAT;
                break;
            }
            if (peek() == 'e' || peek() == 'E')
            { // 'e'/'E'
                buffer_append(&lex, &len, &cap, peek());
                advance();
                state = STATE_PRE_EXP;
                break;
            }
            if (isdigit(peek()))
            { // 0-9
                buffer_append(&lex, &len, &cap, peek());
                advance();
                break;
            }
            return make_token(TOK_INT, lex);

        case STATE_IN_STRING:
            if (peek() == '\n' || peek() == EOF)
            { // EOL / EOF
                return make_error("Unterminated string literal");
            }
            if (peek() == '\\')
            { // '\'
                advance();
                state = STATE_ESC;
                break;
            }
            if (peek() == '"')
            { // '"'
                advance();
                state = STATE_STRING;
                break;
            }
            if (peek() > 31)
            { // Prinatble ASCII
                buffer_append(&lex, &len, &cap, peek());
                advance();
                break;
            }
            return make_error("Invalid control character in string");

        case STATE_ESC:
            if (peek() == EOF)
            { // EOF
                return make_error("Unterminated escape sequence");
            }
            char out;
            switch (peek())
            {
            case 'n':
                buffer_append(&lex, &len, &cap, '\n');
                advance();
                state = STATE_IN_STRING;
                break;
            case 'r':
                buffer_append(&lex, &len, &cap, '\r');
                advance();
                state = STATE_IN_STRING;
                break;
            case 't':
                buffer_append(&lex, &len, &cap, '\t');
                advance();
                state = STATE_IN_STRING;
                break;
            case '\\':
                buffer_append(&lex, &len, &cap, '\\');
                advance();
                state = STATE_IN_STRING;
                break;
            case '"':
                buffer_append(&lex, &len, &cap, '"');
                advance();
                state = STATE_IN_STRING;
                break;

            case 'x':
            { // \xdd hex escape
                advance();

                int h1 = peek();
                advance();

                int h2 = peek();
                advance();

                if (!isxdigit(h1) || !isxdigit(h2))
                {
                    free(lex);
                    return make_error("Invalid hex escape \\x??");
                }

                unsigned value = 0;
                sscanf((char[]){h1, h2, 0}, "%x", &value);
                buffer_append(&lex, &len, &cap, (char)value);
                state = STATE_IN_STRING;
                break;
            }

            default:
                free(lex);
                return make_error("Invalid escape sequence in string");
            }
            break;

        case STATE_STRING:
            if (peek() == '"')
            { // '"'
                if (lex == "\"\"")
                { // """" -> not a single line string but a multi line one in fact
                    buffer_append(&lex, &len, &cap, '\"');
                    advance();
                    WSResult ws = skip_whitespace();
                    state = STATE_MULTIL_STRING;
                    break;
                }
            }
            return make_token(TOK_STRING, lex);

        case STATE_MULTIL_STRING:
            

            if (peek() == EOF)
            {
                return make_error("Unterminated multi-line string");
            }

            if (peek() == ' ' || peek() == '\t' || peek() == '\r')
            {
                advance();
            }

            else if (peek() == '\n')
            {
                buffer_append(&lex, &len, &cap, '\n');
                advance();
                break;
            }

            if (peek() == '"')
            { // '"'
                advance();
                state = STATE_SINGLE_QUATATIONM;
                break;
            }

        case STATE_SINGLE_QUATATIONM:
            if (peek() == '"')
            { // '"'
                advance();
                state = STATE_DOUBLE_QUATATIONM;
                break;
            }
            buffer_append(&lex, &len, &cap, '\"');
            advance();
            state = STATE_MULTIL_STRING;
            break;

        case STATE_DOUBLE_QUATATIONM:
            if (peek() == '"')
            { // '"'
                advance();
                state = STATE_STRING;
                break;
            }

            buffer_append(&lex, &len, &cap, '\"');
            buffer_append(&lex, &len, &cap, '\"');

            advance();
            state = STATE_MULTIL_STRING;
            break;
        }
    }
}