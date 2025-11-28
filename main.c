#include <stdio.h>
#include <stdlib.h>
#include "./src/parser.h"
#include "./src/ast.h"
#include "./src/err.h"
#include "./src/symtable.h"
#include "./src/sem_analysis.h"
#include "./src/args.h"

SymTable *g_global_symtable = NULL;


int main(int argc, char* argv[]) {
    Args args = handle_args(argc, argv);

     g_global_symtable = symtable_create(NULL);
    if (!g_global_symtable)
        error_exit(99, "Out of memory (global symtable)\n");

    scanner_init(args.src_file_path);

    
    ASTNode *root = parser_prog();
    if(sem_analyze(root));
    
    //code_gen(root);


    symtable_free(g_global_symtable);
    g_global_symtable = NULL;

    return 0;
}
