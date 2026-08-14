//
// Created by nkinder on 8/13/26.
//

#include "exec.h"
#include "parser.h"
#include <linux/limits.h>
#include <fcntl.h>
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


int execc(Command* command) {
  pid_t pid = fork();

  if (pid < 0) {
    return 1;
  } else if (pid == 0) {
    if (command->nredirs != 0) {
      int fd = open(command->redirs[0].target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        return 1;
      }
      if (dup2(fd, STDOUT_FILENO) < 0) {
        return 1;
      }
      close(fd);

      execvp(command->argv[0], command->argv);

      return 1;
    } else {
      execvp(command->argv[0], command->argv);
      perror("execvp");
      exit(1);
    }
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

size_t count_command_args(char** argv) {
  size_t len = 0;
  char** iter = argv;
  while (*iter != nullptr) {
    ++iter;
    ++len;
  }

  return len;
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
      Command* command = build_command(tokens);
      BuiltinCmd* cmd = find_builtin(command->argv[0]);


      if (cmd) {
        //WordNode* arg = tokens->head;
        char** args = command->argv;
        //for (int i = 0; i < tokens->size; ++i) {
        //  args[i] = arg->value;
        //  arg = arg->next;
        //}
        int saved_stdout = 0;
        int fd = 0;
        if (command->nredirs != 0) {
          for (int i = 0; i < command->nredirs; ++i) {
            Redirect redirect = command->redirs[i];
            saved_stdout = dup(STDOUT_FILENO);
            fd = open(redirect.target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            exit_status |= cmd->builtin((const int)count_command_args(command->argv), (const char**)command->argv);

            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
          }
        } else {
          exit_status = cmd->builtin((const int)count_command_args(command->argv), (const char**)command->argv);
        }


      } else {
        char cmd_path[PATH_MAX];
        char* res = find_command(cmd_path, command->argv[0]);
        if (res) {
          exit_status = execc(command);
        } else {
          printf("%s: command not found\n", tokens->head->value);
        }
      }
      cleanup_wordlist(tokens);
    }
  }
}



//void prepare_args(char** dest, WordList* words) {
//  WordNode* iter = words->head;
//  for (int i = 0; i < words->size; ++i) {
//    memcpy(dest[i], iter->value, strlen(iter->value) + 1);
//    iter = iter->next;
//  }
//  dest[words->size] = nullptr;
//}
