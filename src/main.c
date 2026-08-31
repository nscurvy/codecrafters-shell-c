#define TMPDISABLED
#undef TMPDISABLED
#include "completion.h"
#include <readline/readline.h>

struct HashTable;

extern struct HashTable* variable_table;


int repl();
#ifdef TMPDISABLED
#include "common.h"
#include "parser.h"


WordList* generate_one_pipe_wordlist() {
  WordList* wordlist = empty_wordlist();
  append_wordlist(wordlist, "echo");
  append_wordlist(wordlist, "hello");
  append_wordlist(wordlist, "world");
  append_wordlist(wordlist, "|");
  append_wordlist(wordlist, "ls");
  append_wordlist(wordlist, ".");
  return wordlist;
}

WordList* generate_two_pipe_wordlist() {
  WordList* wordlist = empty_wordlist();
  append_wordlist(wordlist, "echo");
  append_wordlist(wordlist, "hello");
  append_wordlist(wordlist, "world");
  append_wordlist(wordlist, "|");
  append_wordlist(wordlist, "ls");
  append_wordlist(wordlist, ".");
  append_wordlist(wordlist, "|");
  append_wordlist(wordlist, "echo");
  return wordlist;
}

#endif


int main(int argc, char *argv[]) {
#ifndef TMPDISABLED
  // Flush after every printf
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  rl_attempted_completion_function = shell_completion_function;


  return repl();
#else
  WordList* wordlist = generate_two_pipe_wordlist();

  Pipeline* pipeline = build_pipeline(wordlist);

  cleanup_wordlist(wordlist);
  cleanup_pipeline(pipeline);
  return 0;

#endif
}

