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
#include <readline/history.h>
#include <readline/readline.h>
#define TMPDISABLED

// TODO: docs
char* find_command(char* dest, const char* command) {
  const char* name = "PATH";
  const char* env_p = getenv(name);
  char* result = nullptr;
  if (env_p) {
    char path_buffer[PATH_MAX + 1];
    WordList* path = tokenize_path(env_p);
    WordNode* p = path->head;
    if (p) {
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


// TODO: docs
int execc(const Command* command) {
  pid_t pid = fork();

  if (pid < 0) {
    return 1;
  } else if (pid == 0) {
    if (command->nredirs != 0) {
      int redirected_fd = command->redirs[0].fd;
      unsigned modeflag = (unsigned)command->redirs[0].mode;
      int fd = open(command->redirs[0].target, O_WRONLY | O_CREAT | modeflag, 0644);
      if (fd < 0) {
        return 1;
      }
      if (dup2(fd, redirected_fd) < 0) {
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

// TODO: DOdocs
BuiltinCmd* find_builtin(const char* name) {
  BuiltinCmd* cmd;

  cmd = bsearch(&name, builtins, NUMBUILTINS, sizeof(BuiltinCmd), &pstrcmp);
  return cmd;
}

// TODO: DOdocs
size_t count_command_args(char** argv) {
  size_t len = 0;
  char** iter = argv;
  while (*iter != nullptr) {
    ++iter;
    ++len;
  }

  return len;
}

// TODO: DOdocs
int repl() {
  int exit_status = 0;

  while (true) {
    const char* input_line = readline("$ ");
    if (input_line && strlen(input_line) > 0) {
      add_history(input_line);
      WordList* tokens = tokenize_input(input_line);
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
        //for (int i = 0; i < tokens->size; ++i) {
        //  args[i] = arg->value;
        //  arg = arg->next;
        //}
        int saved_fd = 0;
        int fd = 0;
        if (command->nredirs != 0) {
          for (int i = 0; i < command->nredirs; ++i) {
            Redirect redirect = command->redirs[i];
            int redirected_fd = redirect.fd;
            saved_fd = dup(redirected_fd);
            unsigned truncflag = (unsigned)redirect.mode;
            fd = open(redirect.target, O_WRONLY | O_CREAT | truncflag, 0644);
            dup2(fd, redirected_fd);
            close(fd);

            exit_status |= cmd->builtin((const int)count_command_args(command->argv), (const char**)command->argv);

            dup2(saved_fd, redirected_fd);
            close(saved_fd);
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
      cleanup_command(command);
      free((void*)input_line);
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
