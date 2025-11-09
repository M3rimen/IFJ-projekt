#include <stdio.h>
#include "./src/context.h"


int main(int argc, char* argv[]) {
    CompilerContext* ctx = ctx_init();
    if (!ctx) {
        printf("Failed to initialize compiler context\n");
        return 1;
    }

    ctx->args = handle_args(argc, argv);
    ctx->reader = fr_init(ctx->args.src_file_path);

    printf("File opened: %s\n", ctx->args.src_file_path);
    printf("first char: %c\n", fr_read_char(ctx->reader));

    // LexicalAnalyzer la = LA_init(src);
    // Token token;

    // SyntaxTree tree = SynA_init();
    // SemanticContext sem_ctx = SemA_init();

    // // === Lexical + Syntax + Semantic combined flow ===
    // while ((token = LA_next_token(&la)).type != TOKEN_EOF) {
    //    if (ErrorHandler_has_error()) {
    //         ErrorHandler_report();
    //         cleanup_all();
    //         return 1;
    //     }
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

    //     if (ErrorHandler_has_error()) {
    //         ErrorHandler_report();
    //         cleanup_all();
    //         return 1;
    //     }
    // }

    // Once finished ????? Or include in aloop and generate after every block?
    // IntermediateCode ic = IC_gen(&tree);
    // Optimizer_optimize(&ic);
    // CodeGen_generate(&ic, params.output_path);

    ctx_cleanup(ctx);
    ctx = NULL;
    return 0;
}

