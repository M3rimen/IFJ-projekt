#include "scanner.h"
#include "syntax_analyzer.h"
#include "semantic_analyzer.h"
#include "error_handler.h"

typedef struct Parser {
    Scanner* scanner;
    // SyntaxAnalyzer* syntaxAnalyzer;
    // SemanticAnalyzer* semanticAnalyzer;
    // ErrorHandler* errorHandler;

    // MemoryManager* memoryManager;
    // SymbolTable* symbolTable;
    // SymbolTable* symbolTable;
} Parser;


Parser* parser_init();
void parser_cleanup(Parser* parser);