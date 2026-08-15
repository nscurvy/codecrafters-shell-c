#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <errno.h>
#include "builtins.h"
#include "parser.h"
#define TMPDISABLED
#include <readline/readline.h>

#include "completion.h"


int repl();

//int tokenize_input(char** dest, char* input);


int main(int argc, char *argv[]) {
  // Flush after every printf
  rl_attempted_completion_function = shell_completion_function;


  return repl();
}

