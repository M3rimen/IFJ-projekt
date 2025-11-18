#include <stdio.h>
#include "parser.h"
//#include "scanner.h"

/*
This is basically main parent object of our compiler
*/

Parser* parser_init(char* src_file_path) {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;

    //parser->scanner = scanner_init(src_file_path);
    return parser;
}

parser_parse(Parser* parser) {
    //Token token = scanner_next(parser->scanner);

    // while ((token = LA_next_token(&la)).type != TOKEN_EOF) {
    //     SynA_feed_token(&parser, token); 
    // 
    //     if (SynA_block_complete(&parser)) { 
    //          Tree* tree = SynA_build_tree(&parser); 
    //          SemA_validate(&sem_ctx, tree); 
    //          trees_add(&tree_list, tree); 
    //         SynA_reset_for_next_block(&parser); 
    //     } 
    // }
    // Handle any remaining partial tree at EOF 
    // if (SynA_has_partial_tree(&parser)) { 
    //     Tree* last_tree = SynA_build_tree(&parser); 
    //     SemA_validate(&sem_ctx, last_tree); trees_add(&tree_list, last_tree); }

    // }

    
    //return token;
}
void parser_cleanup(Parser* parser) {
    if (!parser) return;

    // if (parser->scanner) {
    //     scanner_cleanup(parser->scanner);
    //     parser->scanner = NULL;
    // }

    free(parser);
    parser = NULL;
}