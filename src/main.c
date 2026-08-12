#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

typedef struct Command {
  const char* command;
  int argc;
  char** argp;
} Command;

static constexpr size_t ARGC_MAX = 50;

int repl();

int tokenize_input(char** dest, char* input);

Command* make_command(char* command, int argc, char** argp);

void cleanup_command(Command* command);

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);


  return repl();
}

int repl() {
    char input[1024];
    char* args[ARGC_MAX] = {{}};
    bool running = true;
    while (running) {
      printf("$ ");
      setbuf(stdout, NULL);
      const char* input_line = fgets(input, sizeof(input), stdin);
      if (input_line && !ferror(stdin)) { // Check to make sure some error hasn't occured before
        Command* command = nullptr;
        // trusting that command has a sane value
        // Rewmoving to use newline as token delimiter
        //command[strcspn(command, "\n")] = '\0'; // Remove the newline.
        int tokenc = tokenize_input(args, input);
        if (tokenc < 0) {
          perror("tokenize_input failed: ");
          exit(errno);
        }
        if (tokenc > 1) {
          command = make_command(args[0], tokenc - 1, &args[1]);
        } else {
          command = make_command(args[0], 0, nullptr);
        }
        if (strncmp(command->command, "exit", 4) == 0) {
          exit(EXIT_SUCCESS);
        } else if (strcmp(command->command, "echo") == 0) {
          for (int i = 0; i < command->argc; ++i) {
            printf("%s", command->argp[i]);
            if (i == command->argc - 1) {
              putc('\n', stdout);
            } else {
              putc(' ', stdout);
            }
          }
        } else {
          printf("%s: command not found\n", command->command);
          setbuf(stdout, NULL);
        }
        cleanup_command(command);
      }
    }
    return 0;
}

/**
 *
 * @param dest buffer to hold the tokens
 * @param input the raw input line
 * @return the number of tokens created.
 */
int tokenize_input(char** dest, char* input) {
  errno = 0;
  strtok(input, "\n");

  dest[0] = strtok(input, " ");
  int i = 1;
  while (true) {
    char* tmp = strtok(nullptr, " ");
    if (tmp == nullptr || strcmp(tmp, "") == 0) {
      break;
    }
    if (i == ARGC_MAX) {
      errno = EINVAL;
      return -1;
    }
    dest[i++] = tmp;
  }

  return i;
}

Command* make_command(char* command, int argc, char** argp) {
  errno = 0;
  Command* result = malloc(sizeof(Command));
  if (!result) {
    return nullptr;
  }

  result->argc = argc;
  char* cmd = strdup(command);
  if (!cmd) {
    free(result);
    return nullptr;
  }
  result->command = cmd;
  if (result->argc == 0) {
    result->argp = malloc(sizeof(char*));
    result->argp[0] = calloc(1, sizeof(char));
    result->argc = 1;
    return result;
  }
  result->argp = malloc(sizeof(char*) * argc);
  if (!result->argp) {
    free(result->command);
    free(result);
    return nullptr;
  }

  for (int i = 0; i < argc; ++i) {
    char* arg = strdup(argp[i]);

    if (!arg) {
      // Cleanup loop in case of allocation failure
      for (int j = 0; j < i; ++j) {
        free(result->argp[j]);
      }
      free(result->argp);
      free(result->command);
      free(result);
      return nullptr;
    }
    result->argp[i] = arg;
  }

  return result;
}

void cleanup_command(Command* command) {
  for (int i = 0; i < command->argc; ++i) {
    free(command->argp[i]);
  }
  if (command->argc > 0) {
    free(command->argp);
  }
  free(command);
}
