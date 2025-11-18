#include <stdio.h>
#include "./src/args.h"


int main(int argc, char* argv[]) {
    Args args = handle_args(argc, argv);

    Parser* parser = parser_init(args.src_file_path);

    parser_parse(parser);
    parser_cleanup(parser);

    // Once finished ????? Or include in aloop and generate after every block?
    // IntermediateCode ic = IC_gen(&tree);
    // Optimizer_optimize(&ic);
    // CodeGen_generate(&ic, params.output_path);

    return 0;
}

// int main(int argc, char* argv[]) {
//     Args args = handle_args(argc, argv);
    
//     // 1. Lexical + Syntax + Semantic analysis (combined)
//     Parser* parser = parser_init(args.src_file_path);
//     if (!parser) return 99; // internal error
    
//     AST* tree = parser_parse(parser); // builds complete AST
//     if (has_errors()) {
//         return get_error_code(); // returns 1, 2, 3, 4, 5, 6, or 10
//     }
    
//     // 2. Generate complete IFJcode25 AFTER parsing everything
//     CodeGenerator* codegen = codegen_init();
//     codegen_generate(codegen, tree); // outputs to stdout
    
//     // 3. Cleanup
//     parser_cleanup(parser);
//     codegen_cleanup(codegen);
    
//     return 0;
// }

