#include "error.h"
#include <stdarg.h>

void error_msg(const char *msg) {
    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
}

void error_msgf(const char *fmt, ...) {
    if (!fmt) return;

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
