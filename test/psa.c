#include "psa.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

static const PrecAction prec_table[9][9] = {
/*              DOLL   OPER   LPAR   RPAR   MUL    ADD    REL    IS     EQ     */
/* DOLLAR */   {UD,    LW,    LW,    UD,    LW,    LW,    LW,    LW,    LW},
/* OPERAND*/   {HG,    UD,    UD,    HG,    HG,    HG,    HG,    HG,    HG},
/* LPAR   */   {UD,    LW,    LW,    EQ,    LW,    LW,    LW,    LW,    LW},
/* RPAR   */   {HG,    UD,    UD,    HG,    HG,    HG,    HG,    HG,    HG},
/* MUL    */   {HG,    LW,    LW,    HG,    HG,    HG,    HG,    HG,    HG},
/* ADD    */   {HG,    LW,    LW,    HG,    LW,    HG,    HG,    HG,    HG},
/* REL    */   {HG,    LW,    LW,    HG,    LW,    LW,    HG,    HG,    HG},
/* IS     */   {HG,    LW,    LW,    HG,    LW,    LW,    LW,    HG,    HG},
/* EQ     */   {HG,    LW,    LW,    HG,    LW,    LW,    LW,    LW,    HG}
};

typedef struct PsaStackItem {
    Token *token;
    PrecTokenType ptype;
    bool is_nonterm;
    bool is_marker;
    struct PsaStackItem *next;
} PsaStackItem;

typedef PsaStackItem *PsaStack;

static PsaStack stack_init(void) {
    PsaStack head = calloc(1, sizeof(PsaStackItem));
    if (!head) return NULL;
    head->token = NULL;
    head->ptype = PREC_DOLLAR;
    head->is_nonterm = false;
    head->is_marker = false;
    head->next = NULL;
    return head;
}

static void stack_delete(PsaStack head) {
    while (head) {
        PsaStack tmp = head;
        head = head->next;
        free(tmp);
    }
}

static int stack_push(PsaStack *head, Token *tok, PrecTokenType ptype, bool is_nonterm) {
    PsaStack item = malloc(sizeof(PsaStackItem));
    if (!item) return ERR_INTERNAL;
    item->token = tok;
    item->ptype = ptype;
    item->is_nonterm = is_nonterm;
    item->is_marker = false;
    item->next = *head;
    *head = item;
    return ERR_OK;
}

static int stack_insert_marker_before_top_terminal(PsaStack *head) {
    if (!head || !*head) return ERR_INTERNAL;

    PsaStack prev = NULL;
    PsaStack cur = *head;

    while (cur && (cur->is_nonterm || cur->is_marker)) {
        prev = cur;
        cur = cur->next;
    }

    if (!cur) return ERR_INTERNAL;

    PsaStack marker = malloc(sizeof(PsaStackItem));
    if (!marker) return ERR_INTERNAL;

    marker->token = NULL;
    marker->ptype = PREC_DOLLAR;
    marker->is_nonterm = false;
    marker->is_marker = true;
    marker->next = cur;

    if (prev) prev->next = marker;
    else *head = marker;

    return ERR_OK;
}

static PrecTokenType convert_prec_type(Token *tok) {
    if (!tok) return PREC_DOLLAR;

    switch (tok->type) {
        case TOK_IDENTIFIER:
        case TOK_GID:
        case TOK_INT:
        case TOK_FLOAT:
        case TOK_HEX:
        case TOK_STRING:
            return PREC_OPERAND;

        case TOK_KEYWORD:
            if (strcmp(tok->lexeme, "is") == 0) return PREC_IS;
            if (strcmp(tok->lexeme, "null") == 0 ||
                strcmp(tok->lexeme, "true") == 0 ||
                strcmp(tok->lexeme, "false") == 0)
                return PREC_OPERAND;
            return PREC_DOLLAR;

        case TOK_LPAREN: return PREC_LPAR;
        case TOK_RPAREN: return PREC_RPAR;

        case TOK_STAR:  return PREC_MUL;
        case TOK_PLUS:
        case TOK_MINUS: return PREC_ADD;

        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE: return PREC_REL;

        case TOK_EQ:
        case TOK_NE: return PREC_EQ;

        default: return PREC_DOLLAR;
    }
}

static PrecTokenType head_prec_type(PsaStack head) {
    while (head) {
        if (!head->is_nonterm && !head->is_marker)
            return head->ptype;
        head = head->next;
    }
    return PREC_DOLLAR;
}

static bool dpda_finished(PrecTokenType lookahead, PsaStack head) {
    return lookahead == PREC_DOLLAR &&
           head &&
           head->is_nonterm &&
           head->ptype == PREC_OPERAND &&
           head->next &&
           head->next->ptype == PREC_DOLLAR &&
           head->next->next == NULL;
}

static bool newline_should_end_expression(PsaStack stack) {
    PrecTokenType top = head_prec_type(stack);

    switch (top) {
        case PREC_OPERAND:
        case PREC_RPAR:
            return true;

        case PREC_MUL:
        case PREC_ADD:
        case PREC_REL:
        case PREC_IS:
        case PREC_EQ:
        case PREC_LPAR:
            return false;

        default:
            return true;
    }
}

#define S1 (*head)
#define S2 (S1 ? S1->next : NULL)
#define S3 (S2 ? S2->next : NULL)

static int reduce(PsaStack *head) {
    if (!head || !*head) return ERR_INTERNAL;

    PsaStack cur = *head;
    int count = 0;

    while (cur && !cur->is_marker) {
        count++;
        cur = cur->next;
    }

    if (!cur) return ERR_SYNTAX;

    PsaStack marker = cur;
    PsaStack after_marker = marker->next;

    /* Redukce 1 symbolu: E -> id / literal / operand */
    if (count == 1) {
        if (!S1 || S1->is_nonterm || S1->is_marker || S1->ptype != PREC_OPERAND)
            return ERR_SYNTAX;

        Token *t = S1->token;

        PsaStack free_it = *head;
        while (free_it != after_marker) {
            PsaStack next = free_it->next;
            free(free_it);
            free_it = next;
        }

        PsaStack newE = malloc(sizeof(PsaStackItem));
        if (!newE) return ERR_INTERNAL;

        newE->token = t;
        newE->ptype = PREC_OPERAND;
        newE->is_nonterm = true;
        newE->is_marker = false;
        newE->next = after_marker;

        *head = newE;
        return ERR_OK;
    }

    /* Redukce 3 symbolů: E -> ( E ) nebo E -> E op E */
    if (count == 3) {
        if (!S1 || !S2 || !S3) return ERR_SYNTAX;

        Token *t = NULL;

        /* E -> ( E ) */
        if (!S1->is_nonterm && !S1->is_marker && S1->ptype == PREC_RPAR &&
            S2->is_nonterm && S2->ptype == PREC_OPERAND &&
            !S3->is_nonterm && !S3->is_marker && S3->ptype == PREC_LPAR)
        {
            t = S2->token;
        }
        /* E -> E op E */
        else if (S1->is_nonterm && S1->ptype == PREC_OPERAND &&
                 !S2->is_nonterm && !S2->is_marker &&
                 (S2->ptype == PREC_MUL ||
                  S2->ptype == PREC_ADD ||
                  S2->ptype == PREC_REL ||
                  S2->ptype == PREC_IS  ||
                  S2->ptype == PREC_EQ) &&
                 S3->is_nonterm && S3->ptype == PREC_OPERAND)
        {
            t = S2->token; /* len pre „symboliku“, AST neriešime */
        }
        else {
            return ERR_SYNTAX;
        }

        PsaStack free_it = *head;
        while (free_it != after_marker) {
            PsaStack next = free_it->next;
            free(free_it);
            free_it = next;
        }

        PsaStack newE = malloc(sizeof(PsaStackItem));
        if (!newE) return ERR_INTERNAL;

        newE->token = t;
        newE->ptype = PREC_OPERAND;
        newE->is_nonterm = true;
        newE->is_marker = false;
        newE->next = after_marker;

        *head = newE;
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

int parse_expression_psa(GetTokenFn get_token,
                         AdvanceTokenFn advance_token,
                         void *user_data)
{
    if (!get_token || !advance_token) return ERR_INTERNAL;

    PsaStack stack = stack_init();
    if (!stack) return ERR_INTERNAL;

    int res = ERR_OK;
    Token *tok = get_token(user_data);
    PrecTokenType look = convert_prec_type(tok);

    while (!dpda_finished(look, stack)) {

        if (tok->type == TOK_EOL) {
            if (newline_should_end_expression(stack)) {
                look = PREC_DOLLAR;
            } else {
                advance_token(user_data);
                tok = get_token(user_data);
                look = convert_prec_type(tok);
                continue;
            }
        }

        PrecTokenType top = head_prec_type(stack);
        PrecAction act = prec_table[top][look];

        if (act == UD) {
            res = ERR_SYNTAX;
            break;
        }

        if (act == LW) {
            res = stack_insert_marker_before_top_terminal(&stack);
            if (res != ERR_OK) break;
        }

        if (act == LW || act == EQ) {
            if (look == PREC_DOLLAR) {
                res = ERR_SYNTAX;
                break;
            }

            res = stack_push(&stack, tok, look, false);
            if (res != ERR_OK) break;

            advance_token(user_data);
            tok = get_token(user_data);
            look = convert_prec_type(tok);
        }
        else if (act == HG) {
            res = reduce(&stack);
            if (res != ERR_OK) break;
        }
    }

    stack_delete(stack);
    return res;
}
