// err.h
#ifndef ERR_H
#define ERR_H

// Kódy chýb
typedef enum {
    ERR_OK              = 0,   // bez chyby

    ERR_LEX             = 1,   // lexikálna chyba
    ERR_SYN             = 2,   // syntaktická chyba

    ERR_SEM_UNDEF       = 3,   // nedefinovaná funkcia / premenná
    ERR_SEM_REDEF       = 4,   // redefinícia
    ERR_SEM_PARAM       = 5,   // zly počet/typ argumentov
    ERR_SEM_TYPE        = 6,   // typová nekompatibilita
    ERR_SEM_OTHER       = 10,  // ostatné sémantické

    ERR_RUNTIME_UNDEF   = 25,  // behová – zlý typ parametra vstavanej funkcie
    ERR_RUNTIME_TYPE    = 26,  // behová – typová nekompatibilita vo výrazoch

    ERR_INTERNAL        = 99   // interná chyba prekladača (napr. malloc)
} ErrorCode;

// Vytlačí chybovú hlášku na stderr a ukončí program s daným kódom
void error_exit(ErrorCode code, const char *fmt, ...);

#endif // ERR_H
