#define TMPDISABLED
#include "completion.h"
#include <readline/readline.h>



int repl();



int main(int argc, char *argv[]) {
  // Flush after every printf
  rl_attempted_completion_function = shell_completion_function;


  return repl();
}

