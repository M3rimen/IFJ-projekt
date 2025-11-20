#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "scanner.h"
#include "parser.h"

// Lokálna deklarácia aby GCC nevrešťal
void scanner_init(FILE *in);

typedef struct {
    const char *name;
    const char *input;
    int expected_exit;
} Test;


// =============================
//   ZÁKLADNÉ TESTY GRAMATIKY
// =============================
Test tests[] = {

    // =========================================
    // 1. VALIDNÉ PROGRAMY
    // =========================================

    {
        "validny minimalny program",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "}\n",
        0
    },

    {
        "validna prazdna function_sig",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static run() {\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validna funkcia s parametrom",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f(a) {\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny getter",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static value {\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny setter",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static set = (x) {\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny var + expr",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static m() {\n"
        "var x = 5\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny return literal",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static r() {\n"
        "return 5\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny ID call bez parametrov",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static r() {\n"
        "foo()\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny ID call s parametrom",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static r() {\n"
        "foo(5)\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny if-else so simple expr",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static r() {\n"
        "if (x) {\n"
        "} else {\n"
        "}\n"
        "}\n"
        "}\n",
        0
    },

    {
        "validny while",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static w() {\n"
        "while (x) {\n"
        "}\n"
        "}\n"
        "}\n",
        0
    },

    // =========================================
    // 2. CHYBY V PROLOGU
    // =========================================

    {
        "chyba import keyword",
        "imprt \"ifj25\" for Ifj\n"
        "class Program {\n}\n",
        2
    },

    {
        "zly string v importe",
        "import \"ifk25\" for Ifj\n"
        "class Program {\n}\n",
        2
    },

    {
        "chyba for keyword",
        "import \"ifj25\"\n"
        "class Program {\n}\n",
        2
    },

    {
        "chyba Ifj keyword",
        "import \"ifj25\" for Xfj\n"
        "class Program {\n}\n",
        2
    },

    // =========================================
    // 3. CHYBY CLASS_DEF
    // =========================================

    {
        "chyba class keyword",
        "import \"ifj25\" for Ifj\n"
        "clas Program {\n}\n",
        2
    },

    {
        "chyba mena Program",
        "import \"ifj25\" for Ifj\n"
        "class Prog {\n}\n",
        2
    },

    {
        "chyba LBRACE",
        "import \"ifj25\" for Ifj\n"
        "class Program \n"
        "}\n",
        2
    },

    {
        "chyba EOL_M za {",
        "import \"ifj25\" for Ifj\n"
        "class Program { static f(){}\n"
        "}\n",
        2
    },

    // =========================================
    // 4. CHYBY FUNCTION_DEF
    // =========================================

    {
        "chyba static keyword",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "statt f() {\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba ID po static",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static 123 {\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba function_kind",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f X\n"
        "}\n",
        2
    },

    // =========================================
    // 5. CHYBY STATEMENTS
    // =========================================

    {
        "chyba var bez ID",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f() {\n"
        "var\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba ID tail assign bez expr",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f() {\n"
        "x =\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba volania foo(",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f() {\n"
        "foo(\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba return + chyba expr",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static r() {\n"
        "return =\n"
        "}\n"
        "}\n",
        2
    },

    {
        "if bez else",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f() {\n"
        "if (x) {\n"
        "}\n"
        "}\n"
        "}\n",
        2
    },

    {
        "chyba while bez )",
        "import \"ifj25\" for Ifj\n"
        "class Program {\n"
        "static f() {\n"
        "while (x {\n"
        "}\n"
        "}\n"
        "}\n",
        2
    }

};



// ======================================================
// SPUSTENIE JEDNÉHO TESTU S PIPE + STDERR CAPTURE
// ======================================================
int run_test(const char *input, char *errbuf, size_t buflen)
{
    int inpipe[2];
    int errpipe[2];

    pipe(inpipe);
    pipe(errpipe);

    write(inpipe[1], input, strlen(input));
    close(inpipe[1]);

    int backup_stdin  = dup(0);
    int backup_stderr = dup(2);

    dup2(inpipe[0], 0);     // presmeruj stdin
    dup2(errpipe[1], 2);    // presmeruj stderr

    close(inpipe[0]);
    close(errpipe[1]);

    int status = 0;
    pid_t pid = fork();

    if (pid == 0) {
        scanner_init(stdin);       // !!! MUSÍ BYŤ !!!
        parser_prog();
        exit(0);
    }

    // v rodičovi čítame chybové hlášky
    close(errpipe[1]);
    ssize_t n = read(errpipe[0], errbuf, buflen - 1);
    if (n > 0)
        errbuf[n] = '\0';
    else
        errbuf[0] = '\0';

    close(errpipe[0]);

    waitpid(pid, &status, 0);

    // obnovíme stdin a stderr
    dup2(backup_stdin, 0);
    dup2(backup_stderr, 2);
    close(backup_stdin);
    close(backup_stderr);

    return WEXITSTATUS(status);
}


// ========================
//           MAIN
// ========================
int main()
{
    int total = sizeof(tests) / sizeof(Test);
    int ok = 0;

    for (int i = 0; i < total; i++) {

        char err[2048];

        int ret = run_test(tests[i].input, err, sizeof(err));

        printf("=== Test: %s ===\n", tests[i].name);

        if (ret == tests[i].expected_exit) {
            printf("[OK] exit=%d\n", ret);
            ok++;
        } else {
            printf("[FAIL] exit=%d (expected %d)\n", ret, tests[i].expected_exit);
        }

        if (strlen(err) > 0) {
            printf("Chybová hláška parsera:\n%s\n", err);
        } else {
            printf("Chybová hláška parsera: <žiadna>\n");
        }

        printf("\n");
    }

    printf("=== Výsledok: %d/%d OK ===\n", ok, total);
    return 0;
}
