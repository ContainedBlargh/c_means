#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

/**
 * @brief Checks if a char exists in a string.
 *
 * @param string The string to check.
 * @param character The char to look for.
 *
 * @return Whether the char is contained within the string.
 */
bool char_in_string(char* string, char character);

/**
 * @brief Replace all instances of a character in a string.
 *
 * @param string The string to replace in.
 * @param before The char to replace.
 * @param after The char to replace with.
 */
void char_replace(char* string, char before, char after);

#endif
