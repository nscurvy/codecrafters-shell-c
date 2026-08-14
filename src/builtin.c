//
// Created by nkinder on 8/13/26.
//
#include "builtins.h"
#include "parser.h"
#include "exec.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int builtin_exit(int argc, char** argv);
int builtin_echo(int argc, char** argv);
int builtin_type(int argc, char** argv);
const BuiltinCmd builtins[NUMBUILTINS] = {
  { "echo", &builtin_echo},
  {"exit", &builtin_exit},
  {"type", &builtin_type}
};

int builtin_exit(int argc, char** argv) {
  exit(EXIT_SUCCESS);
}

int builtin_echo(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    fputs(argv[i], stdout);
    if (i < argc - 1) {
      fputs(" ", stdout);
    } else {
      fputs("\n", stdout);
    }
  }
  setbuf(stdout, nullptr);
  return 0;
}

int builtin_type(int argc, char** argv) {
  if (argc >= 2) {
    BuiltinCmd* cmd = find_builtin(argv[1]);
    if (cmd == nullptr) {
      char buf[PATH_MAX + 1] = {0};
      char* executable = find_command(buf, argv[1]);
      if (executable) {
        printf("%s is %s\n", argv[1], buf);
      } else {
        printf("%s: not found\n", argv[1]);
      }
    } else {
      printf("%s is a shell builtin\n", argv[1]);
    }
  }
  return 0;
}

int pstrcmp(const void* a, const void* b) {
  return strcmp(*(const char* const *)a, *(const char *const * ) b);
}

