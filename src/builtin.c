//
// Created by nkinder on 8/13/26.
//
#define __STDC_WANT_LIB_EXT1__ 1
#include "builtins.h"
#include "exec.h"
#include "parser.h"

#include <errno.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int builtin_cd(const int argc, const char** argv);
int builtin_exit(const int argc, const char **argv);
int builtin_echo(const int argc, const char **argv);
int builtin_type(const int argc, const char **argv);
int builtin_pwd(const int argc, const char **argv);
const BuiltinCmd builtins[NUMBUILTINS] = {
    {"cd", &builtin_cd }, {"echo", &builtin_echo}, {"exit", &builtin_exit},  {"pwd", &builtin_pwd},{"type", &builtin_type}
};

int builtin_cd(const int argc, const char** argv) {
  errno = 0;
  const char* target;
  int result = 0;
  if (argc > 1) {
    target = argv[1];
  } else {
    errno = ENOENT;
    result = -1;
    return result;
  }

  result = chdir(target);
  if (result == -1) {

    const char* errmsg = strerror(ENOENT);
    printf("cd: %s: %s\n", target, errmsg);
  }
  return result;
}

int builtin_exit(const int argc, const char **argv) { exit(EXIT_SUCCESS); }

int builtin_echo(const int argc, const char **argv) {
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

int builtin_type(const int argc, const char **argv) {
  if (argc >= 2) {
    BuiltinCmd *cmd = find_builtin(argv[1]);
    if (cmd == nullptr) {
      char buf[PATH_MAX + 1] = {0};
      char *executable = find_command(buf, argv[1]);
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

int builtin_pwd(const int argc, const char** argv) {
  char buf[PATH_MAX];

  if (getcwd(buf, PATH_MAX) != nullptr) {
    printf("%s\n", buf);
  }

  return 0;
}

int pstrcmp(const void *a, const void *b) {
  return strcmp(*(const char *const *)a, *(const char *const *)b);
}
