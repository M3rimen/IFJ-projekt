#pragma once
#include <stdio.h>

typedef struct FileReader {
    FILE *file;
    int line;
    int col;
    int last_char;
} FileReader;

FileReader* fr_init(const char *path);
int fr_read_char(FileReader *fr);
void fr_cleanup(FileReader *fr);