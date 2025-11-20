#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

/* ZÁKLADNÉ CHYBOVÉ KÓDY */
#define ERR_OK        0      // všetko OK
#define ERR_LEX       1      // lexikálna chyba
#define ERR_SYNTAX    2      // syntaktická chyba
#define ERR_SEM       3      // semantická chyba
#define ERR_INTERNAL 99      // interná chyba prekladača

/* jednorázový výpis chyby (nevykonáva exit!) */
void error_msg(const char *msg);

/* výpis chyby s formátom (nevykonáva exit!) */
void error_msgf(const char *fmt, ...);

#endif
