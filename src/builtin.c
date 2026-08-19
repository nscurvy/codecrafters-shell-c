//
// Created by nkinder on 8/13/26.
//
#define __STDC_WANT_LIB_EXT1__ 1
#include "common.h"
#include "builtins.h"
#include "exec.h"
#include "expand.h"
#include "parser.h"

#include "completion.h"
#include "jobs.h"

int builtin_cd(const int argc, const char** argv);
int builtin_exit(const int argc, const char **argv);
int builtin_echo(const int argc, const char **argv);
int builtin_type(const int argc, const char **argv);
int builtin_pwd(const int argc, const char **argv);
int builtin_complete(const int argc, const char **argv);
int builtin_jobs(const int argc, const char **argv);
const BuiltinCmd builtins[NUMBUILTINS] = {
  {.name = "cd", .builtin = &builtin_cd },
  {.name = "complete", .builtin = &builtin_complete },
  {.name = "echo", .builtin = &builtin_echo},
  {.name = "exit", .builtin = &builtin_exit},
    {.name = "jobs", .builtin = &builtin_jobs},
  {.name = "pwd", .builtin = &builtin_pwd},
  {.name = "type", .builtin = &builtin_type}
};

int builtin_complete(const int argc, const char **argv) {
  if (argc >= 2 && strcmp(argv[1], "-p") == 0) {
    if (argc < 3) {
      return 1;
    }
    const char* path = lookup_completion(argv[2]);
    if (path) {
      fprintf(stdout, "complete -C '%s' %s\n", path, argv[2]);
    } else {
      fprintf(stderr, "%s: %s: no completion specification\n", argv[0], argv[2]);
      return 1;

    }
    return 0;
  }

  if (argc >= 2 && strcmp(argv[1], "-C") == 0) {
    if (argc < 4) {
      fprintf(stderr, "complete: -C requires a script path and command\n");
      return 1;
    }
    register_completion(argv[3], argv[2]);
    return 0;
  }

  if (argc >= 2 && strcmp(argv[1], "-r") == 0) {
    if (argc < 3) {
      fprintf(stderr, "complete: -r requires a command\n");
      return 1;

    }
    unregister_completion(argv[2]);
  }

  return 1;
}

int builtin_jobs(const int argc, const char **argv) {
  if (argc == 1) {
    print_jobs();
  }
  return 0;
}

int builtin_cd(const int argc, const char** argv) {
  char buf[PATH_MAX] = {0};
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
  //if (target[0] == '~') {
  //  char* expanded = expand_home(buf, target);
  //  result = chdir(expanded);
  //} else {
  //  result = chdir(target);
  //}
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
