#include <stdio.h>
#include "src/args.c"


int main(int argc, char* argv[]) {
    Args args = handle_args(argc, argv);


    //FILE* src = open_file(params.src_path);
    printf("File opened: %s\n", args.src_file_path);

    // LexicalAnalyzer la = LA_init(src);
    // Token token;

    // SyntaxTree tree = SynA_init();
    // SemanticContext sem_ctx = SemA_init();

    // // === Lexical + Syntax + Semantic combined flow ===
    // while ((token = LA_next_token(&la)).type != TOKEN_EOF) {
    //     SynA_feed_token(&tree, token);  // Incrementally build/validate syntax tree
    //     SemA_feed_token(&sem_ctx, &tree, token); // Check semantics as we go

    //     if (ErrorHandler_has_error()) {
    //         ErrorHandler_report();
    //         cleanup_all();
    //         return 1;
    //     }
    // }

    // // Once finished:
    // IntermediateCode ic = IC_gen(&tree);
    // Optimizer_optimize(&ic);
    // CodeGen_generate(&ic, params.output_path);

    // cleanup_all();
    // return 0;
}