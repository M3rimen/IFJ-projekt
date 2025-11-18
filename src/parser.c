#include "parser.h"

/*
This is basically main parent object of our compiler
*/

Parser* parser_init() {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;

    parser->scanner = NULL;
    return parser;
}

void parser_cleanup(Parser* parser) {
    if (!parser) return;

    if (parser->scanner) {
        scanner_cleanup(parser->scanner);
        parser->scanner = NULL;
    }

    free(parser);
}