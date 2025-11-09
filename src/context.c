typedef struct CompilerContext {
    // Lexer *lexer;
    // SyntaxTree *syntax_tree;
    // SemanticContext *semantic;
    // TokenArray *tokens;
    // FILE *source_file;
    // // ... add more as needed
} CompilerContext;


// void cleanup_all(CompilerContext *ctx) {
//     if (ctx->tokens) token_array_free(ctx->tokens);
//     if (ctx->syntax_tree) syn_free(ctx->syntax_tree);
//     if (ctx->lexer) la_free(ctx->lexer);
//     if (ctx->semantic) sem_free(ctx->semantic);
//     if (ctx->source_file) fclose(ctx->source_file);
//     free(ctx);
// }