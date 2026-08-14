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



int repl();

//int tokenize_input(char** dest, char* input);


int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);


  return repl();
}

