#ifndef AST_H
#define AST_H

#include <stdbool.h>

/*
 * Základné typy AST uzlov.
 *
 * Zatiaľ sú tu:
 *   - literály (int, float, string, bool, null)
 *   - identifikátor
 *   - binárny operátor (E op E)
 *
 * Ďalšie typy (if, while, priradenie, volanie funkcie, blok, ...) môžeš
 * doplniť neskôr.
 */
typedef enum {
    AST_INT_LIT,
    AST_FLOAT_LIT,
    AST_STRING_LIT,
    AST_BOOL_LIT,
    AST_NULL_LIT,

    AST_IDENT,

    AST_BINOP

    /* neskôr napr.:
     * AST_UNOP,
     * AST_ASSIGN,
     * AST_CALL,
     * AST_IF,
     * AST_WHILE,
     * AST_RETURN,
     * AST_BLOCK,
     * AST_FUNC_DEF,
     * AST_PROGRAM
     */
} AstNodeType;

/*
 * Typy binárnych operátorov, ktoré PSA pozná:
 *   +, -, *, /, <, <=, >, >=, ==, !=, is
 */
typedef enum {
    AST_OP_ADD,
    AST_OP_SUB,
    AST_OP_MUL,
    AST_OP_DIV,

    AST_OP_LT,
    AST_OP_LE,
    AST_OP_GT,
    AST_OP_GE,

    AST_OP_EQ,
    AST_OP_NE,

    AST_OP_IS
} AstBinOpKind;

/*
 * Všeobecný AST uzol.
 *
 *  - left/right: podvýrazy (pri binárnych operátoroch, priradení, atď.)
 *  - next: univerzálne prepojenie do zoznamu (napr. zoznam príkazov,
 *          zoznam argumentov, parametre funkcie, atď.)
 *  - line: číslo riadka (z tokenu) – hodí sa pre hlásenia chýb
 */
typedef struct ASTNode {
    AstNodeType type;

    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;

    int line;

    union {
        struct {
            long long value;
        } int_lit;

        struct {
            double value;
        } float_lit;

        struct {
            char *value;   /* vlastnený string (malloc), alebo len pointer do lexému podľa tvojho dizajnu */
        } string_lit;

        struct {
            bool value;
        } bool_lit;

        struct {
            /* null nič nenesie, line je dosť */
            int dummy;
        } null_lit;

        struct {
            char *name;    /* meno identifikátora */
        } ident;

        struct {
            AstBinOpKind op;
        } binop;

        /* neskôr sem môžeš doplniť struct-y pre ďalšie typy uzlov */
    } data;
} ASTNode;


/* ==== Konštruktory pre základné uzly (mimo PSA) ==== */

ASTNode *ast_make_int_lit(long long value, int line);
ASTNode *ast_make_float_lit(double value, int line);
ASTNode *ast_make_string_lit(const char *value, int line);
ASTNode *ast_make_bool_lit(bool value, int line);
ASTNode *ast_make_null_lit(int line);
ASTNode *ast_make_ident(const char *name, int line);
ASTNode *ast_make_binary(AstBinOpKind op,
                         ASTNode *left,
                         ASTNode *right,
                         int line);

/* Uvoľnenie celého AST stromu (rekurzívne) */
void ast_free(ASTNode *node);

/* Voliteľne, debug výpis AST (ak chceš): */
void ast_print(const ASTNode *node, int indent);

#endif /* AST_H */
