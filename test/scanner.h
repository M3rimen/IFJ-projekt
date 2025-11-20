// scanner.h
#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include <stdio.h>

void scanner_init(FILE *input);
Token scanner_next();


typedef enum {
    STATE_START,

    STATE_PRE_GID,
    STATE_GID,

    STATE_ID,

    STATE_SINGLE_ZERO,

    STATE_PRE_HEX,
    STATE_HEX,

    STATE_PRE_FLOAT,
    STATE_FLOAT,

    STATE_PRE_EXP,
    STATE_EXP,

    STATE_INT,

    STATE_PRE_STRING,
    STATE_IN_STRING,
    STATE_ESC,
    STATE_MULTIL_STRING
} LexerState;



#endif