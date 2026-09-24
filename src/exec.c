//
// Created by nkinder on 8/13/26.
//

#include "exec.h"
#include "common.h"
#include "expand.h"
#include "jobs.h"
#include "parser.h"
#include "path.h"
#include "vars.h"

#include <readline/history.h>
#include <readline/readline.h>

#define TMPDISABLED

size_t
count_command_args(const char**);

/*
// TODO: docs
char*
find_command(char* dest, const char* command) {
    const char* name   = "PATH";
    const char* env_p  = getenv(name);
    char*       result = nullptr;
    if (env_p) {
        char      path_buffer[PATH_MAX + 1];
        TokenList* path = tokenize_path(env_p);
        Token* p    = path->head;
        if (p) {
            for (int i = 0; i < path->size; ++i) {
                memset(path_buffer, 0, sizeof(path_buffer));
                strcpy(path_buffer, p->value);
                strcat(path_buffer, "/");
                strcat(path_buffer, command);
                struct stat buffer;

                if (stat(path_buffer, &buffer) == 0) {
                    // printf("%s is %s\n", command, path_buffer);
                    if ((buffer.st_mode & S_IXUSR) || (buffer.st_mode & S_IXGRP) || (buffer.st_mode & S_IXOTH)) {
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
        tokenlist_delete(path);
    }
    return result;
}
*/

int
exec_builtin(Command* command, BuiltinCmd* cmd) {
    return 0;
    int saved_fd    = 0;
    int fd          = 0;
    int exit_status = 0;
    if (command->nredirs != 0) {
        for (int i = 0; i < command->nredirs; ++i) {
            Redirect redirect      = command->redirs[i];
            int      redirected_fd = redirect.fd;
            saved_fd               = dup(redirected_fd);
            int truncflag          = (int) redirect.mode;
            fd                     = open(redirect.target, O_WRONLY | O_CREAT | truncflag, 0644);
            dup2(fd, redirected_fd);
            close(fd);

            exit_status |= cmd->builtin((const int) count_command_args((const char**) command->argv),
                                        (const char**) command->argv);

            dup2(saved_fd, redirected_fd);
            close(saved_fd);
        }
    } else {

        cmd->builtin((const int) count_command_args((const char**) command->argv), (const char**) command->argv);
    }
    return exit_status;
}


int
execute_pipes(Pipeline* pipeline) {
    pid_t            pids[pipeline->size];
    int              pipes[pipeline->size - 1][2];
    size_t           n         = pipeline->size;
    size_t           num_pipes = pipeline->size - 1;
    PipelineElement* iter      = pipeline->head;
    for (size_t i = 0; i < pipeline->size; ++i) {
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
            Command*    command = iter->command;
            BuiltinCmd* builtin = find_builtin(command->argv[0]);

            if (builtin) {
                exit(exec_builtin(command, builtin));
            }

            char* res = path_find_command(command->argv[0]);
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
        iter = iter->next;
    }

    for (size_t j = 0; j < num_pipes; ++j) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }

    for (size_t i = 0; i < n; ++i) {
        waitpid(pids[i], nullptr, 0);
    }
    return 0;
}


int
exec_pipe(Command* first, Command* second) {
    int         fds[2];
    pid_t       pid_first;
    pid_t       pid_second;
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
            exit(exec_builtin(first, cmd1));
        } else {

            char* res = path_find_command(first->argv[0]);
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
            exit(exec_builtin(second, cmd2));
        } else {
            char* res = path_find_command(second->argv[0]);
            if (res) {
                execvp(second->argv[0], second->argv);
            } else {
                printf("%s: command not found\n", second->argv[0]);
                free(res);
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

int
execute_builtin(Command* command) {
    BuiltinCmd* becmd = find_builtin(command->argv[0]);
    if (becmd) {
        int argc = (int) count_command_args((const char**) command->argv);
        return becmd->builtin(argc, (const char**) command->argv);
    }
    return -1;
}

bool
is_builtin(Command* command) {
    return find_builtin(command->argv[0]) != nullptr;
}

bool
check_command(Command* command) {
    if (command->argv[0] == nullptr) {
        return false;
    }
    return true;
}

int
execute_pipeline(Pipeline* pipeline) {
    int status = 0;

    if (pipeline->size == 1) {
        if (check_command(pipeline->head->command)) {
            if (is_builtin(pipeline->head->command)) {
                status = execute_builtin(pipeline->head->command);
            } else {
                status = execute_command(pipeline->head->command);
            }
        }
    } else {
        status = execute_pipes(pipeline);
    }

    return status;
}

int
execute_command(Command* command) {
    pid_t pid = fork();

    if (pid == 0) {
        for (size_t i = 0; i < command->nredirs; ++i) {
            int target_fd;
            switch (command->redirs[i].mode) {
            case REDIR_APPEND:
                target_fd = open(command->redirs[i].target, O_WRONLY | O_APPEND | O_CREAT);
                if (command->redirs[i].fd == 1) {
                    dup2(target_fd, STDOUT_FILENO);
                    close(target_fd);
                } else if (command->redirs[i].fd == 2) {
                    dup2(target_fd, STDERR_FILENO);
                    close(target_fd);
                }
                break;
            case REDIR_OUT:
                target_fd = open(command->redirs[i].target, O_WRONLY | O_TRUNC | O_CREAT);
                if (command->redirs[i].fd == 1) {
                    dup2(target_fd, STDOUT_FILENO);
                    close(target_fd);
                } else if (command->redirs[i].fd == 2) {
                    dup2(target_fd, STDERR_FILENO);
                    close(target_fd);
                }
                break;
            case REDIR_IN:
                target_fd = open(command->redirs[i].target, O_RDONLY);
                if (command->redirs[i].fd != 0) {
                    _exit(1);
                } else {
                    dup2(target_fd, STDIN_FILENO);
                    close(target_fd);
                }
                break;
            case REDIR_HEREDOC:
            case REDIR_ERR:
                _exit(1);
            }
        }
        const char* cmd = path_find_command(command->argv[0]);
        if (!cmd) {
            fprintf(stderr, "Command not found: %s\n", command->argv[0]);
            _exit(127);
        } else {
            execvp(command->argv[0], command->argv);
            perror("execvp");
            _exit(127);
        }
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return status;
    } else {
        perror("Error forking");
        _exit(127);
    }
    return 0;
}

int
execute_andor(AndOr* andor) {
    int status = 0;

    AndOrElement* iter = andor->head;
    while (iter != nullptr) {
        switch (iter->op) {
        case AND_NONE:
            status = execute_pipeline(iter->pipeline);
            break;
        case AND_AND:
            if (status != 0) {
                return status;
            } else {
                status = execute_pipeline(iter->pipeline);
            }
            break;
        case AND_OR:
            if (status == 0) {
                return status;
            } else {
                status = execute_pipeline(iter->pipeline);
            }
            break;
        }
        iter = iter->next;
    }

    return status;
}

int
execute_andor_bg(AndOr* andor) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork process.");
        return pid;
    } else if (pid > 0) {
        const char* cmdline = join_andor(andor);
        register_job(pid, cmdline);
        free((void*) cmdline);
        return 0;
    } else {
        int status = execute_andor(andor);
        _exit(status);
    }
}


void
expand_list(List* list);

int
execute_list(List* list) {
    int status = 0;
    expand_list(list);

    ListElement* iter = list->head;
    while (iter != nullptr) {
        switch (iter->sep) {
        case SEP_SEMI:
        case SEP_NONE:
            status = execute_andor(iter->and_or);
            break;
        case SEP_AMP:
            status = execute_andor_bg(iter->and_or);
            break;
        }
        iter = iter->next;
    }

    return status;
}

void
execute_assignment(Assignment* assignment) {
    const char* val = assignment->value;
    char*       pair[2];
    assignment_split(pair, val);
    var_assign(pair[0], pair[1]);
    free(pair[0]);
    free(pair[1]);
}

void
expand_redirection(Redirect* redirect) {
    const char* expansion = expand_word(redirect->target);
    const char* old       = redirect->target;
    redirect->target      = (char*) expansion;
    free(old);
}

void
expand_redirections(Redirect* redirs, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        expand_redirection(&redirs[i]);
    }
}

void
expand_assignment(Assignment* item) {
    const char* expansion = expand_word(item->value);
    const char* old       = item->value;
    item->value           = expansion;
    free(old);
}

void
expand_args(char** argv) {
    char** iter = argv;
    while (*iter != nullptr) {
        char* expansion = (char*) expand_word(*iter);
        char* old       = *iter;
        *iter           = expansion;
        free(old);
        ++iter;
    }
}

void
expand_assignments(Assignment* list) {
    Assignment* iter = list;
    while (iter != nullptr) {
        expand_assignment(iter);
        execute_assignment(iter);
        iter = iter->next;
    }
}

void
expand_command(Command* command) {
    expand_assignments(command->assignment_list);
    expand_redirections(command->redirs, command->nredirs);
    expand_args(command->argv);
}

void
expand_pipeline(Pipeline* pipeline) {
    PipelineElement* iter = pipeline->head;
    while (iter != nullptr) {
        expand_command(iter->command);
        iter = iter->next;
    }
}

void
expand_andor(AndOr* andor) {
    AndOrElement* iter = andor->head;
    while (iter != nullptr) {
        expand_pipeline(iter->pipeline);
        iter = iter->next;
    }
}

void
expand_list(List* list) {
    ListElement* iter = list->head;
    while (iter != nullptr) {
        expand_andor(iter->and_or);
        iter = iter->next;
    }
}


int
exec_pipeline(Pipeline* pipeline) {
    return 0;
    /*
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
      */
}


// TODO: docs
int
execc(const Command* command) {
    return 0;
    /*
      char  cmd_path[PATH_MAX];
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
              int      redirected_fd = command->redirs[0].fd;
              unsigned modeflag      = (unsigned) command->redirs[0].mode;
              int      fd            = open(command->redirs[0].target, O_WRONLY | O_CREAT | modeflag, 0644);
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
              append_job(pid, (const char**) command->argv);
          }
      }

      return 0;
      */
}

// TODO: DOdocs
BuiltinCmd*
find_builtin(const char* name) {
    BuiltinCmd* cmd;

    cmd = (BuiltinCmd*) bsearch(&name, builtins, NUMBUILTINS, sizeof(BuiltinCmd), &pstrcmp);
    return cmd;
}

// TODO: DOdocs
size_t
count_command_args(const char** argv) {
    size_t       len  = 0;
    const char** iter = argv;
    while (*iter != nullptr) {
        ++iter;
        ++len;
    }

    return len;
}

void
exit_handler() {
    const char* histfile = getenv("HISTFILE");
    if (histfile) {
        int status = write_history(histfile);
        if (status == -1) {
            fprintf(stderr, "Failed to write history to file %s\n", histfile);
        }
    }
}


// TODO: DOdocs
int
repl() {
    // return 0;
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, nullptr);
    var_init_from_environ();
    using_history();
    atexit(exit_handler);
    rl_event_hook        = check_background_jobs;
    const char* histfile = getenv("HISTFILE");
    if (histfile) {
        int status = read_history(histfile);
        if (status == -1) {
            fprintf(stderr, "Failed to read history from file %s\n", histfile);
        }
    }

    int exit_status = 0;

    while (true) {
        check_background_jobs();
        const char* input_line = readline("$ ");
        if (input_line && strlen(input_line) > 0) {
            add_history(input_line);
            List* list = lex_and_parse(input_line);

            if (!list) {
                perror("Error with parsing the input.");
            }
            exit_status = execute_list(list);

            list_delete(list);
            free((void*) input_line);
        }
    }
}


// void prepare_args(char** dest, WordList* words) {
//   WordNode* iter = words->head;
//   for (int i = 0; i < words->size; ++i) {
//     memcpy(dest[i], iter->value, strlen(iter->value) + 1);
//     iter = iter->next;
//   }
//   dest[words->size] = nullptr;
// }
