//
// Created by nkinder on 9/10/26.
//

#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool has_path_components(const char* command) {
  size_t len = strlen(command);
  const char* iter = command;
  bool result = false;

  while (*iter != '\0') {
     if (*iter == '/') {
       result = true;
       break;
     }
    ++iter;
  }
  return result;
}

WordList*
path_split(const char* path_value) {
  WordList* result = wordlist_new_empty();

  const char* iter = path_value;
  while (*iter != '\0') {
    const char* component_begin = iter;
    const char* component_end = component_begin;
    while (*iter != ':' && *iter != '\0') {
      ++component_end;
      ++iter;
    }
    if (*iter == ':') {
      size_t distance = (size_t) (component_end - component_begin + 1);
      char buf[distance];
      memmove(buf, component_begin, distance - 1);
      buf[distance - 1] = '\0';
      wordlist_append(result, buf);
    }
    if (*iter == '\0') {
      size_t distance = (size_t) (component_end - component_begin + 1);
      char buf[distance];
      memmove(buf, component_begin, distance - 1);
      buf[distance - 1] = '\0';
      wordlist_append(result, buf);
      break;
    }

    ++iter;
  }
  return result;
}

const char*
path_getenv() {
  const char* path = getenv("PATH");
  return path;
}

WordList*
path_get_dirs() {
  const char* path = path_getenv();
  if (path == nullptr) {
    return wordlist_new_empty();
  }
  return path_split(path);
}

char*
path_find_command(const char* command) {
  if (has_path_components(command)) {
    if (path_is_executable_file(command)) {
      return strdup(command);
    } else {
      return nullptr;
    }
  }
  char* result = nullptr;
  WordList* path_components = path_get_dirs();

  Word* iter = path_components->head;

  while (iter != nullptr) {
    char* resolved = path_join(iter->text, command);
    if (path_is_executable_file(resolved)) {
      result = resolved;
      break;
    }
    free(resolved);
    iter = iter->next;;
  }
  wordlist_delete(path_components);
  return result;
}

bool
path_is_executable_file(const char* path) {
  struct stat st;

  if (stat(path, &st) != 0) {
    return false;
  }

  if (!S_ISREG(st.st_mode)) {
    return false;
  }

  return access(path, X_OK) == 0;
}

char*
path_join(const char* dir, const char* name) {
  char* result = nullptr;
  const char* dir_actual = nullptr;
  if (strcmp(dir, "") == 0) {
      dir_actual = ".";
  } else {
    dir_actual = dir;
  }
  size_t numchars = strlen(dir_actual) + strlen(name) + 2;
  result = calloc(numchars, sizeof(char));
  char* destiter = result;
  strcpy(result, dir_actual);
  destiter += strlen(dir_actual);
  *destiter = '/';
  ++destiter;
  memcpy(destiter, name, strlen(name));

  return result;
}