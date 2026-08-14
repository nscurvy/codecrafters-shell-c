//
// Created by nkinder on 8/13/26.
//

#include "expand.h"
#include <stdlib.h>
#include <string.h>

char *expand_home(char *buf, const char *path) {
  char* buff_iter = buf;
  char* homepath = getenv("HOME");

  memmove(buff_iter, homepath, strlen(homepath));
  buff_iter += strlen(homepath);
  *buff_iter = '/';
  ++buff_iter;
  strncat(buff_iter, &path[1], strlen(&path[1]));
  buff_iter += strlen(&path[1]) + 1;

  return buf;
}