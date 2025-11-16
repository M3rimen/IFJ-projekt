// token.h
#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    // identifiers & literals
    TOK_IDENTIFIER,
    TOK_GID,
    TOK_KEYWORD,
    TOK_INT,
    TOK_FLOAT,
    TOK_HEX,
    TOK_STRING,
    TOK_BUILTIN_CALL, // Ifj.<id>

    // operators
    TOK_PLUS,   // +
    TOK_MINUS,  // -
    TOK_STAR,   // *
    TOK_ASSIGN, // =
    TOK_EQ,     // ==
    TOK_LT,     // <
    TOK_LE,     // <=
    TOK_GT,     // >
    TOK_GE,     // >=
    TOK_NE,     // !=

    // punctuation
    TOK_LPAREN,    // (
    TOK_RPAREN,    // )
    TOK_LBRACE,    // {
    TOK_RBRACE,    // }
    TOK_COMMA,     // ,
    TOK_DOT,       // .
    TOK_SEMICOLON, // ;

    // special
    TOK_EOF,
    TOK_EOL,
    TOK_ERROR
} TokenType;

typedef struct
{
    TokenType type;
    char *lexeme;
} Token;

#endif