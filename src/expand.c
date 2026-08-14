//
// Created by nkinder on 8/13/26.
//

#include "expand.h"

#include "parser.h"

#include <stdlib.h>
#include <string.h>

size_t exptilde(char *dest, QuoteFlagE *flag) {
  char *tilde_value;
  int chars_wrote = 0;
  switch (*flag) {
  case UNQUOTED:
    tilde_value = getenv("HOME");
    break;
  case SINGLE_QUOTED:
  case DOUBLE_QUOTED:
    tilde_value = "~";
    break;
  default:
    return -1;
  }

  memmove(dest, tilde_value, strlen(tilde_value));
  const char *i = dest + strlen(tilde_value);
  chars_wrote = i - dest;
  return chars_wrote;
}

char *exptok(const char *word) {
  char buf[1024] = {0};
  QuoteFlagE flag = UNQUOTED;
  const char *iter = word;
  size_t i = 0;

  while (*iter != '\0') {
    char c = *iter;

    switch (c) {
    case '\'':
      switch (flag) {
      case UNQUOTED:
        flag = SINGLE_QUOTED;
        break;
      case SINGLE_QUOTED:
        flag = UNQUOTED;
        break;
      case DOUBLE_QUOTED:
        buf[i++] = c;
        break;
      }
      break;
    case '"':
      switch (flag) {
      case UNQUOTED:
        flag = DOUBLE_QUOTED;
        break;
      case SINGLE_QUOTED:
        buf[i++] = c;
        break;
      case DOUBLE_QUOTED:
        flag = UNQUOTED;
        break;
      }
      break;
    case '~':
      i += exptilde(&buf[i], &flag);
      break;
    default:
      buf[i++] = c;
      break;
    }
    ++iter;
  }

  char *ret = calloc(strlen(buf) + 1, sizeof(char));
  if (!ret) {
    return nullptr;
  }

  strcpy(ret, buf);

  return ret;
}

char *expand_home(char *buf, const char *path) {
  char *buff_iter = buf;
  char *homepath = getenv("HOME");

  memmove(buff_iter, homepath, strlen(homepath));
  buff_iter += strlen(homepath);
  *buff_iter = '/';
  ++buff_iter;
  strncat(buff_iter, &path[1], strlen(&path[1]));
  buff_iter += strlen(&path[1]) + 1;

  return buf;
}