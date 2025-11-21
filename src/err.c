/**
 * @file err.c
 * @author Marko Tokár (xtokarm00)
 * @brief 
 * @version 0.1
 * @date 2025-11-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


void error_exit(int exit_type, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(exit_type);
}