//
// Created by nkinder on 9/12/26.
//

#pragma once
#include <stddef.h>

typedef struct StringBuilder {
  size_t capacity;
  size_t size;
  char* str;
} StringBuilder;

/**
 * Initializes a new StringBuilder with the given requested capacity.
 *
 * @param initial_capacity The initial capacity of the StringBuilder
 * @return A newly allocated StringBuilder or nullptr
 */
StringBuilder* sb_new_sized(size_t initial_capacity);
/**
 * Convenience default constructor which sets the capacity to the
 * reasonable default of 16.
 */
#define sb_new() sb_new_sized(16)

/**
 * Append a string to the StringBuilder.
 *
 * @param sb StringBuilder
 * @param str string
 */
void sb_appends(StringBuilder* sb, const char* str);
/**
 * Append a char to the StringBuilder.
 *
 * @param sb StringBuilder
 * @param c char
 */
void sb_appendc(StringBuilder* sb, char c);
/**
 * Append a long to the buffer.
 *
 * @param sb StringBuilder
 * @param l long
 */
void sb_appendl(StringBuilder* sb, long l);
/**
 * Append a format string to the back of the StringBuilder. This will resize the buffer
 * as needed to accommodate the formatted string. Unless memory allocation fails, this
 * function should always succeed, in that it should eventually allocate a large enough
 * buffer.
 *
 * @param sb StringBuilder
 * @param fmt Format string
 * @param ... Format args
 * @return Number of written characters
 */
int sb_format(StringBuilder* sb, const char* fmt, ...);

/**
 * Take the stored buffer from the StringBuilder. This makes it so the stored
 * string doesn't need to be copied. The StringBuilder will generate a new zeroed buffer
 * of the old capacity.
 *
 * @param sb The StringBuilder
 * @return The actual buffer stored in the sb
 */
const char* sb_takestring(StringBuilder* sb);

/**
 * Delete a StringBuilder. This doesn't NULL anything.
 *
 * @param sb The StringBuilder to destroy
 */
void
sb_delete(StringBuilder* sb);