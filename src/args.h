#pragma once
#include <stdio.h>

typedef struct Args {
    char* src_file_path;
} Args;

Args handle_args(int argc, char* argv[]);