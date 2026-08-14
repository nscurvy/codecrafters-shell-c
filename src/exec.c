//
// Created by nkinder on 8/13/26.
//

#include "exec.h"
#include "parser.h"
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define TMPDISABLED

char* find_command(char* dest, const char* command) {
  const char* name = "PATH";
  const char* env_p = getenv(name);
  char* result = nullptr;
  if (env_p) {
    char path_buffer[PATH_MAX + 1];
    WordList* path = tokenize_path(env_p);
    WordNode* p = path->head;
    {
      for (int i = 0; i < path->size; ++i) {
        memset(path_buffer, 0, sizeof(path_buffer));
        strcpy(path_buffer, p->value);
        strcat(path_buffer, "/");
        strcat(path_buffer, command);
        struct stat buffer;

        if (stat(path_buffer, &buffer) == 0) {
          //printf("%s is %s\n", command, path_buffer);
          if ((buffer.st_mode & S_IXUSR)
        || (buffer.st_mode & S_IXGRP)
        || (buffer.st_mode & S_IXOTH)) {
            strcpy(dest, path_buffer);
            result = dest;
            goto CLEANUP_WORDS;
        }
        }
        p = p->next;
      }
      memset(dest, 0, 1);
    }
    CLEANUP_WORDS:
      cleanup_wordlist(path);
  }
  return result;
}


int execc(int argc, char** argv) {
  pid_t pid = fork();

  if (pid < 0) {
    return 1;
  } else if (pid == 0) {
    execvp(argv[0], argv);
    perror("execvp");
    exit(1);
  } else {
    int status;
    waitpid(pid, &status, 0);
  }

  return 0;
}

BuiltinCmd* find_builtin(const char* name) {
  BuiltinCmd* cmd;

  cmd = bsearch(&name, builtins, NUMBUILTINS, sizeof(BuiltinCmd), &pstrcmp);
  return cmd;
}

int repl() {
  char input[1024];
  char* args[50] = {{}};
  int exit_status = 0;

  while (true) {
    printf("$ ");
    setbuf(stdout, nullptr);
    const char* input_line = fgets(input, sizeof(input), stdin);

    if (input_line && !ferror(stdin)) {
      WordList* tokens = tokenize_input(input);
      if (!tokens) {
        exit_status = -1;
        setbuf(stderr, nullptr);
        continue;
      }

      WordNode* iter = tokens->head;
      while (iter != nullptr) {
        iter = iter->next;
      }
      BuiltinCmd* cmd = find_builtin(tokens->head->value);

      if (cmd) {
        WordNode* arg = tokens->head;

        for (int i = 0; i < tokens->size; ++i) {
          args[i] = arg->value;
          arg = arg->next;
        }
        exit_status = cmd->builtin((const int)tokens->size, (const char**)args);


      } else {
        char cmd_path[PATH_MAX];
        char* res = find_command(cmd_path, tokens->head->value);
        if (res) {
          char argbuf[50][50];
          char* dest[50];
          for (int i = 0; i < 50; ++i) {
            dest[i] = argbuf[i];
          }
          prepare_args(dest, tokens);
          exit_status = execc(tokens->size, dest);
        } else {
          printf("%s: command not found\n", tokens->head->value);
        }
      }
      cleanup_wordlist(tokens);
    }
  }
}

void prepare_args(char** dest, WordList* words) {
  WordNode* iter = words->head;
  for (int i = 0; i < words->size; ++i) {
    memcpy(dest[i], iter->value, strlen(iter->value) + 1);
    iter = iter->next;
  }
  dest[words->size] = nullptr;
}
