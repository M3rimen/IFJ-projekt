/**
 * @file err.h
 * @author Marko Tokár (xtokarm00)
 * @brief 
 * @version 0.1
 * @date 2025-11-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef ERR_H
#define ERR_H

/**
 * @brief error_exit - prints formatted error message and exits program
 * 
 * @param exit_type which exit code to use
 * @param fmt string format for error message
 * @param ... 
 */
void error_exit(int exit_type, const char *fmt, ...);

#endif // ERR_H