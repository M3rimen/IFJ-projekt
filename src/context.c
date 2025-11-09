#include "context.h"
#include <stdlib.h>

// Initialize
CompilerContext* ctx_init() {
    CompilerContext* ctx = malloc(sizeof(CompilerContext));
    if (!ctx) return NULL;

    // initialize pointers
    ctx->args.src_file_path = NULL;
    ctx->reader = NULL;
    return ctx;
}

// Cleanup
void ctx_cleanup(CompilerContext *ctx) {
    if (!ctx) return;

    if (ctx->reader) {
        fr_cleanup(ctx->reader);
        ctx->reader = NULL;
    }

    free(ctx);
}