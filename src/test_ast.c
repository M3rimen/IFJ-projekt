#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "scanner.h"
#include "ast.h"

//
// PRETTY TREE PRINT HELPERS
//

void print_tree(ASTNode *n, int level, int is_last, int *stack)
{
    if (!n) return;

    // Print prefix branches
    for (int i = 0; i < level; i++) {
        if (stack[i])
            printf("│   ");
        else
            printf("    ");
    }

    // Node bullet
    printf("%s", is_last ? "└── " : "├── ");

    // Print node type
    const char *t = "???";
    switch (n->type) {
        case AST_PROGRAM: t = "PROGRAM"; break;
        case AST_PROLOG: t = "PROLOG"; break;
        case AST_CLASS: t = "CLASS"; break;
        case AST_FUNCTION_S: t = "FUNCTION_S"; break;
        case AST_FUNCTION_DEF: t = "FUNCTION_DEF"; break;
        case AST_FUNCTION: t = "FUNCTION"; break;
        case AST_GETTER: t = "GETTER"; break;
        case AST_SETTER: t = "SETTER"; break;
        case AST_PARAM_LIST: t = "PARAM_LIST"; break;
        case AST_BLOCK: t = "BLOCK"; break;
        case AST_VAR_DECL: t = "VAR"; break;
        case AST_ASSIGN: t = "ASSIGN"; break;
        case AST_CALL: t = "CALL"; break;
        case AST_RETURN: t = "RETURN"; break;
        case AST_IF: t = "IF"; break;
        case AST_ELSE: t = "ELSE"; break;
        case AST_WHILE: t = "WHILE"; break;
        case AST_EXPR: t = "EXPR"; break;
        case AST_IDENTIFIER: t = "IDENTIFIER"; break;
        case AST_GID: t = "GID"; break;
        case AST_LITERAL: t = "LITERAL"; break;
        default: break;
    }

    printf("%s", t);

    if (n->token && n->token->lexeme)
        printf(" [%s]", n->token->lexeme);

    printf("\n");

    // Print children
    for (int i = 0; i < n->child_count; i++) {
        stack[level] = !is_last;
        print_tree(n->children[i], level + 1, i == n->child_count - 1, stack);
    }
}


// -----------------------------------------------------------
// TESTS
// -----------------------------------------------------------

typedef struct {
    const char *name;
    const char *src;
} Test;

Test tests[] = {

    // =====================================================
    {   "Minimalny program",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "}\n"
    },

    // =====================================================
    {   "Program s 1 funkciou a var",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static foo() {\n"
        "  var x = 5\n"
        "  var y\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Getter + Setter",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static value {\n"
        "}\n"
        "static value = (v) {\n"
        "  return v\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "If + Else + While",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static test() {\n"
        "  if (x) {\n"
        "    var a = 1\n"
        "  } else {\n"
        "    var b = 2\n"
        "  }\n"
        "  while (y) {\n"
        "    var c = 3\n"
        "  }\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Funkcia s parametrami",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static sum(a, b, c) {\n"
        "  return a\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Vola sa funkcia vo vnútri bloku",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static main() {\n"
        "  foo(1, 2, 3)\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Priradenie a následné použitie",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static main() {\n"
        "  g = 10\n"
        "  x = g\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Komplexná kombinácia",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static test(a) {\n"
        "  var x = 5\n"
        "  if (x) {\n"
        "    foo(a)\n"
        "    while (g) {\n"
        "      x = 7\n"
        "    }\n"
        "  } else {\n"
        "    return a\n"
        "  }\n"
        "}\n"
        "}\n"
    },

    // =====================================================
    {   "Setter s call + priradenie",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static setX = (v) {\n"
        "  x = v\n"
        "  getY()\n"
        "}\n"
        "}\n"
    }

};



// -----------------------------------------------------------
// MAIN
// -----------------------------------------------------------
int main()
{
    int total = sizeof(tests)/sizeof(Test);

    for (int i = 0; i < total; i++)
    {
        printf("\n========== TEST: %s ==========\n", tests[i].name);

        char tmpfile[] = "/tmp/ifjtestXXXXXX";
        int fd = mkstemp(tmpfile);
        if (fd < 0) { perror("mkstemp"); exit(1); }

        FILE *f = fdopen(fd, "w+");
        fwrite(tests[i].src, 1, strlen(tests[i].src), f);
        rewind(f);

        scanner_init(f);
        ASTNode *root = parser_prog();

        printf("\n--- AST STROM ---\n");
        int stack[200] = {0};
        print_tree(root, 0, 1, stack);
        printf("========== END TEST ==========\n");

        ast_free(root);
        fclose(f);
        unlink(tmpfile);
    }

    return 0;
}
