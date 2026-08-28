//
// Created by nkinder on 8/13/26.
//

#include "common.h"
#include "exec.h"
#include "parser.h"
#include "jobs.h"
#include <readline/history.h>
#include <readline/readline.h>

#define TMPDISABLED

size_t count_command_args(const char**);

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

int exec_builtin(Command* command, BuiltinCmd* cmd) {
  int saved_fd = 0;
  int fd = 0;
  int exit_status = 0;
  if (command->nredirs != 0) {
    for (int i = 0; i < command->nredirs; ++i) {
      Redirect redirect = command->redirs[i];
      int redirected_fd = redirect.fd;
      saved_fd = dup(redirected_fd);
      unsigned truncflag = (unsigned)redirect.mode;
      fd = open(redirect.target, O_WRONLY | O_CREAT | truncflag, 0644);
      dup2(fd, redirected_fd);
      close(fd);

      exit_status |= cmd->builtin((const int)count_command_args((const char**)command->argv), (const char**)command->argv);

      dup2(saved_fd, redirected_fd);
      close(saved_fd);
    }
  } else {
    cmd->builtin((const int)count_command_args((const char**)command->argv), (const char**)command->argv);
  }
  return exit_status;
}


int exec_pipes(Pipeline* pipeline) {
  pid_t pids[pipeline->ncmds];
  int pipes[pipeline->ncmds - 1][2];
  int n = pipeline->ncmds;
  int num_pipes = pipeline->ncmds - 1;
  for (int i = 0; i < pipeline->ncmds; ++i) {
    if (i < num_pipes) {
      pipe(pipes[i]);
    }
    pid_t pid;

    pid = fork();

    if (pid == 0) {
      if (i > 0) {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }
      if (i < n - 1) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }
      for (int j = 0; j < num_pipes; ++j) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      Command* command = pipeline->cmds[i];
      BuiltinCmd*  builtin = find_builtin(command->argv[0]);

      if (builtin) {
        _exit(exec_builtin(command, builtin));
      }

      char cmd_path[PATH_MAX];
      char* res = find_command(cmd_path, command->argv[0]);
      if (res) {
        execvp(command->argv[0], command->argv);
      } else {
        printf("%s: command not found\n", command->argv[0]);
        _exit(127);
      }
    } else if (pid < 0) {
      perror("fork");
      return -1;
    } else {
      pids[i] = pid;
    }


    }

    for (int j = 0; j < num_pipes; ++j) {
      close(pipes[j][0]);
      close(pipes[j][1]);
    }

    for (int i = 0; i < n; ++i) {
      waitpid(pids[i], nullptr, 0);
    }
    return 0;
  }



int exec_pipe(Command* first, Command* second) {
  int fds[2];
  pid_t pid_first;
  pid_t pid_second;
  BuiltinCmd* cmd1 = find_builtin(first->argv[0]);
  BuiltinCmd* cmd2 = find_builtin(second->argv[0]);

  pipe(fds);

  pid_first = fork();

  if (pid_first < 0) {
    perror("fork");
    return -1;
  } else if (pid_first == 0) {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    close(fds[1]);
    if (cmd1) {
      _exit(exec_builtin(first, cmd1));
    } else {
      char cmd_path[PATH_MAX];
      char* res = find_command(cmd_path, first->argv[0]);
      if (res) {
        execvp(first->argv[0], first->argv);
      } else {
        printf("%s: command not found\n", first->argv[0]);
      }
    }
    _exit(127);
  }
  pid_second = fork();

  if (pid_second < 0) {
    perror("fork");
    return -1;
  } else if (pid_second == 0) {
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    close(fds[1]);
    if (cmd2) {
      _exit(exec_builtin(second, cmd2));
    } else {
      char cmd_path[PATH_MAX];
      char* res = find_command(cmd_path, second->argv[0]);
      if (res) {
        execvp(second->argv[0], second->argv);
      } else {
        printf("%s: command not found\n", second->argv[0]);
      }
    }
    _exit(127);
  }
  close(fds[0]);
  close(fds[1]);
  waitpid(pid_first, nullptr, 0);
  waitpid(pid_second, nullptr, 0);
  return 0;
}

int exec_pipeline(Pipeline* pipeline) {
  int exit_status = 0;
  if (pipeline->ncmds != 0) {
    if (pipeline->ncmds == 1) {
      BuiltinCmd* builtin = find_builtin(pipeline->cmds[0]->argv[0]);
      if (builtin) {
        exit_status = exec_builtin(pipeline->cmds[0], builtin);
      } else {
        exit_status = execc(pipeline->cmds[0]);
      }
    } else {
      if (pipeline->ncmds > 1) {
        exit_status = exec_pipes(pipeline);
      }
    }
  }
  return exit_status;
}


// TODO: docs
int execc(const Command* command) {
  char cmd_path[PATH_MAX];
  char* res = find_command(cmd_path, command->argv[0]);
  if (!res) {
    printf("%s: command not found\n", command->argv[0]);
    return -1;
  }
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
    if (!command->bgjob) {
      waitpid(pid, &status, 0);
    } else {
      append_job(pid, (const char**)command->argv);
    }
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
size_t count_command_args(const char** argv) {
  size_t len = 0;
  const char** iter = argv;
  while (*iter != nullptr) {
    ++iter;
    ++len;
  }

  return len;
}

// TODO: DOdocs
int repl() {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, nullptr);
  setbuf(stderr, nullptr);
  setbuf(stdin, nullptr);

  int exit_status = 0;

  while (true) {
    check_background_jobs();
    const char* input_line = readline("$ ");
    if (input_line && strlen(input_line) > 0) {
      add_history(input_line);
      WordList* tokens = tokenize_input(input_line);
      if (!tokens) {
        exit_status = -1;
        continue;
      }

      WordNode* iter = tokens->head;
      while (iter != nullptr) {
        iter = iter->next;
      }
      Pipeline* pipeline = build_pipeline(tokens);
      if (pipeline->ncmds == 0) {
        exit_status = -1;
        continue;
      }

      exec_pipeline(pipeline);

      cleanup_wordlist(tokens);
      cleanup_pipeline(pipeline);
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
