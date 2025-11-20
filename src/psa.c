#include "psa.h"
#include "psa_stack.h"
#include "scanner.h"
#include <string.h>

// -------------------- Operator Precedence Table --------------------
PrecedenceRelation prec_table[9][9] = {
    //        MD     AS     REL    IS     EQ     ID     (      )      $
    /*MD*/   {GT,    GT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*AS*/   {LT,    GT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*REL*/  {LT,    LT,    GT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*IS*/   {LT,    LT,    LT,    GT,    GT,    LT,    LT,    GT,    GT},
    /*EQ*/   {LT,    LT,    LT,    LT,    GT,    LT,    LT,    GT,    GT},
    /*ID*/   {GT,    GT,    GT,    GT,    GT,    UD,    UD,    GT,    GT},
    /*(*/    {LT,    LT,    LT,    LT,    LT,    LT,    LT,    EQ,    UD},
    /*)*/    {GT,    GT,    GT,    GT,    GT,    UD,    UD,    GT,    GT},
    /*$*/    {LT,    LT,    LT,    LT,    LT,    LT,    LT,    UD,    EQ}
};

// -------------------- Token–to–Group Mapping Function --------------------
PrecedenceGroup token_to_group(const Token *tok)
{
    switch (tok->type) {

        // aritmetika
        case TOK_STAR:   // *
        case TOK_SLASH:  // /
            return GRP_MUL_DIV;

        case TOK_PLUS:   // +
        case TOK_MINUS:  // -
            return GRP_ADD_SUB;

        // relačné operátory
        case TOK_LT:     // <
        case TOK_LE:     // <=
        case TOK_GT:     // >
        case TOK_GE:     // >=
            return GRP_REL;

        // rovnostné operátory
        case TOK_EQ:     // ==
        case TOK_NE:     // !=
            return GRP_EQ;

        // zátvorky
        case TOK_LPAREN:
            return GRP_LPAREN;
        case TOK_RPAREN:
            return GRP_RPAREN;

        // koniec vstupu (pseudo-$)
        case TOK_EOF:
            return GRP_EOF;

        // identifikátory a literály
        case TOK_IDENTIFIER:
        case TOK_GID:
        case TOK_INT:
        case TOK_FLOAT:
        case TOK_HEX:
        case TOK_STRING:
            return GRP_ID;

        // kľúčové slová – tu rozlišujeme "is"
        case TOK_KEYWORD:
            if (tok->lexeme && strcmp(tok->lexeme, "is") == 0)
                return GRP_IS;

            return GRP_ID;

        default:
            return GRP_EOF;
    }
}

// -------------------- End-of-expression tokens (bez EOL!) --------------------
static int is_expr_end_token(TokenType t)
{
    return (t == TOK_SEMICOLON ||
            t == TOK_EOF);
}

// pomocná funkcia: je predchádzajúci token "operátor alebo (" ?
// (KEYWORD "is" riešime cez last_is_is_op)
static int is_op_or_lparen(TokenType last_type, int last_is_is_op)
{
    if (last_is_is_op)
        return 1;

    switch (last_type) {
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE:
    case TOK_EQ:
    case TOK_NE:
    case TOK_LPAREN:
        return 1;
    default:
        return 0;
    }
}

// -------------------- Reduce handle (GT case) --------------------
static PsaResult psa_reduce_handle(void)
{
    StackItem handle[4];
    int hlen = 0;

    // pop until MARKER
    while (1)
    {
        if (stack_size() == 0)
            return PSA_ERR_INTERNAL;

        StackItem it = stack_pop();

        if (it.kind == SYM_MARKER)
            break;

        if (hlen >= 4)
            return PSA_ERR_INTERNAL;

        handle[hlen++] = it;
    }

    // E -> ID
    if (hlen == 1 &&
        handle[0].kind == SYM_TERMINAL &&
        handle[0].group == GRP_ID)
    {
        // OK
    }
    // E -> ( E )
    else if (hlen == 3 &&
             handle[0].kind == SYM_TERMINAL &&
             handle[0].tok_type == TOK_RPAREN &&
             handle[1].kind == SYM_NONTERM &&
             handle[2].kind == SYM_TERMINAL &&
             handle[2].tok_type == TOK_LPAREN)
    {
        // OK
    }
    // E -> E op E
    else if (hlen == 3 &&
             handle[0].kind == SYM_NONTERM &&
             handle[1].kind == SYM_TERMINAL &&
             handle[2].kind == SYM_NONTERM)
    {
        PrecedenceGroup g = handle[1].group;
        if (!(g == GRP_MUL_DIV ||
              g == GRP_ADD_SUB ||
              g == GRP_REL     ||
              g == GRP_IS      ||
              g == GRP_EQ))
        {
            return PSA_ERR_SYNTAX;
        }
    }
    else {
        return PSA_ERR_SYNTAX;
    }

    // semantika zatiaľ neriešime, typ necháme TYPE_NONE
    stack_push_nonterm(TYPE_NONE);
    return PSA_OK;
}

// -------------------- Main PSA Expression Parser --------------------
PsaResult psa_parse_expression(Token first, Token *out_next)
{
    stack_init();

    // push $
    Token bottom_tok;
    bottom_tok.type   = TOK_EOF;
    bottom_tok.lexeme = NULL;
    stack_push_terminal(&bottom_tok);

    // prvý token výrazu
    Token current = first;

    if (current.type == TOK_EOF || current.type == TOK_SEMICOLON)
        return PSA_ERR_SYNTAX;

    int use_pseudo_eof = 0;
    Token end_token;

    // pre NL variantu 2 – posledný "významný" token v source
    TokenType last_type = current.type;
    int last_is_is_op =
        (current.type == TOK_KEYWORD &&
         current.lexeme &&
         strcmp(current.lexeme, "is") == 0);

    while (1)
    {
        // -------------------- špeciálne spracovanie EOL --------------------
        if (!use_pseudo_eof && current.type == TOK_EOL) {

            if (is_op_or_lparen(last_type, last_is_is_op)) {
                // newline po operátore / '(' → whitespace, výraz pokračuje
                do {
                    current = scanner_next();
                } while (current.type == TOK_EOL);

                // last_type/last_is_is_op NEAKTUALIZUJEME – update až pri shift-e
                continue;   // nové kolo while s novým current
            }

            // newline po operande alebo ')': výraz končí
            use_pseudo_eof = 1;
            end_token = current;
        }

        StackItem *top_term = stack_top_terminal();
        if (!top_term)
            return PSA_ERR_INTERNAL;

        PrecedenceGroup g_stack = top_term->group;
        PrecedenceGroup g_input;

        if (!use_pseudo_eof)
        {
            if (is_expr_end_token(current.type))
            {
                use_pseudo_eof = 1;
                end_token = current;
                g_input = GRP_EOF;
            }
            else
            {
                g_input = token_to_group(&current);
            }
        }
        else
        {
            g_input = GRP_EOF;
        }

        PrecedenceRelation rel = prec_table[g_stack][g_input];

        // koncový stav: [$, E] a vstup $
        if (use_pseudo_eof && stack_is_eof_with_E_on_top())
        {
            if (rel == EQ)
            {
                if (out_next)
                    *out_next = end_token;
                return PSA_OK;
            }
            else if (rel == GT)
            {
                PsaResult r = psa_reduce_handle();
                if (r != PSA_OK)
                    return r;
                continue;
            }
            else
            {
                return PSA_ERR_SYNTAX;
            }
        }

        switch (rel)
        {
        case LT:
            stack_insert_marker_after_top_terminal();
            if (use_pseudo_eof)
                return PSA_ERR_INTERNAL;
            stack_push_terminal(&current);

            last_type = current.type;
            last_is_is_op = (current.type == TOK_KEYWORD &&
                             current.lexeme &&
                             strcmp(current.lexeme, "is") == 0);

            current = scanner_next();
            break;

        case EQ:
            if (use_pseudo_eof)
                return PSA_ERR_SYNTAX;
            stack_push_terminal(&current);

            last_type = current.type;
            last_is_is_op = (current.type == TOK_KEYWORD &&
                             current.lexeme &&
                             strcmp(current.lexeme, "is") == 0);

            current = scanner_next();
            break;

        case GT:
        {
            PsaResult r = psa_reduce_handle();
            if (r != PSA_OK)
                return r;
            break;
        }

        
        case UD:
        default:
            return PSA_ERR_SYNTAX;
        }
    }
}
