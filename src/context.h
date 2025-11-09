#ifndef CONTEXT_H
#define CONTEXT_H

#include "reader.h"
#include "args.h"

typedef struct CompilerContext {
    Args args;
    FileReader *reader;
} CompilerContext;

// Initialize compiler context
CompilerContext* ctx_init();

// Cleanup compiler context
void ctx_cleanup(CompilerContext *ctx);

#endif // CONTEXT_H

