#include "reader.h"
#include <stdlib.h>

FileReader* fr_init(const char *path) {
    FileReader *fr = malloc(sizeof(FileReader));
    if (!fr) return NULL;

    fr->file = fopen(path, "r");
    if (!fr->file) {
        perror("Failed to open file");
        free(fr);
        return NULL;
    }

    fr->line = 1;
    fr->col = 0;
    fr->last_char = -1;

    return fr;
}

int fr_read_char(FileReader *fr) {
    if (!fr || !fr->file) return EOF;

    int c = fgetc(fr->file);
    if (c == '\n') {
        fr->line++;
        fr->col = 0;
    } else {
        fr->col++;
    }

    fr->last_char = c;
    return c;
}

void fr_cleanup(FileReader *fr) {
    if (!fr) return;
    if (fr->file) {
        fclose(fr->file);
        fr->file = NULL;
    }
    free(fr);
}