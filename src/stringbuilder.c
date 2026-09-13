//
// Created by nkinder on 9/12/26.
//

#include "stringbuilder.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

StringBuilder*
sb_new_sized(size_t initial_capacity) {
  StringBuilder* sb = malloc(sizeof(StringBuilder));
  if (!sb) {
    return nullptr;
  }
  if (initial_capacity == 0) {
    initial_capacity = 16;
  }
  char* buf = calloc(initial_capacity, sizeof(char));
  if (!buf) {
    free(sb);
    return nullptr;
  }
  sb->capacity = initial_capacity;
  sb->size = 0;
  sb->str = buf;
  return sb;
}

void
sb_appends(StringBuilder* sb, const char* str) {
  size_t space_needed = strlen(str);
  if (space_needed >= sb->capacity - (sb->size - 1)) {
    if (sb->capacity == 0) {
      sb->capacity = 16;
    }
    int growth_factor = 1;
    while ((sb->capacity << growth_factor) - sb->size <= space_needed) {
      ++growth_factor;
    }

    sb->capacity <<= growth_factor;
    char* tmp = realloc(sb->str, sb->capacity);
    if (tmp != nullptr) {
      sb->str = tmp;
      char* pos = &sb->str[sb->size];
      memset(pos, 0, sb->capacity - sb->size);
    } else {
      return;
    }
  }
  char* position = &sb->str[sb->size];
  strcat(position, str);
  sb->size += space_needed;
}

void
sb_appendc(StringBuilder* sb, char c) {
  if ((sb->size - 1) == sb->capacity) {
    if (sb->capacity == 0) {
      sb->capacity = 16;
    }
    char* tmp = realloc(sb->str, sb->capacity << 1);
    if (tmp != nullptr) {
      sb->capacity <<= 1;
      sb->str = tmp;
    } else {
      return;
    }
  }
  sb->str[sb->size++] = c;
}

void
sb_appendl(StringBuilder* sb, long l) {
  size_t long_digits;
  if (l >= 0) {
    long_digits = (size_t)log10((double)l) + 1;
  } else {
    long_digits = (size_t)log10((double)-l) + 1 + 1;
  }
  if (long_digits + (sb->size - 1) >= sb->capacity) {
    if (sb->capacity == 0) {
      sb->capacity = 16;
    }
    int growth_factor = 1;
    while ((sb->capacity << growth_factor) - sb->size <= long_digits) {
      ++growth_factor;
    }

    sb->capacity <<= growth_factor;
    char* tmp = realloc(sb->str, sb->capacity);
    if (tmp != nullptr) {
      sb->str = tmp;
      char* pos = &sb->str[sb->size];
      memset(pos, 0, sb->capacity - sb->size);
    } else {
      return;
    }
  }
  char* position = &sb->str[sb->size];
  snprintf(position, sb->capacity - sb->size - 1, "%ld", l);
  sb->size += long_digits;

}
int
sb_format(StringBuilder* sb, const char* fmt, ...) {
  char* current_position = &sb->str[sb->size];
  va_list args, args_copy;

  va_start(args, fmt);
  va_copy(args_copy, args);
  size_t space_remaining  = sb->capacity - sb->size;
  int chars_needed = vsnprintf(current_position, space_remaining, fmt, args);
  int result;
  if (chars_needed >= space_remaining) {
    if (sb->capacity == 0) {
      sb->capacity = 16;
    }
    size_t growth_factor = 1;
    while ((sb->capacity << growth_factor) - sb->size <= chars_needed) {
      ++growth_factor;
    }
    sb->capacity <<= growth_factor;
    char* new_buf = realloc(sb->str, sb->capacity);
    if (new_buf != nullptr) {
      sb->str = new_buf;
      char* pos = &sb->str[sb->size];
      memset(pos, 0, sb->capacity - sb->size);
    } else {
      return -1;
    }

    current_position = &sb->str[sb->size];
    space_remaining = sb->capacity - sb->size;
    result = vsnprintf(current_position, space_remaining, fmt, args_copy);
    sb->size += (size_t)result + 1;

  } else {
    result = chars_needed;
    sb->size += (size_t)result + 1;
  }
  va_end(args);
  return result;
}

const char*
sb_takestring(StringBuilder* sb) {
  const char* result = sb->str;

  sb->str = nullptr;
  sb->size = 0;
  sb->str = calloc(sb->capacity, sizeof(char));
  return result;
}
void
sb_delete(StringBuilder* sb) {
  free(sb->str);
  free(sb);
}
