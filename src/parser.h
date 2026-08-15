//
// Created by nkinder on 8/13/26.
//

#pragma once
#include <stdlib.h>
#include <linux/limits.h>

#define MAX_REDIRS 1;

// TODO: DOCS
typedef enum QuoteFlagE {
  UNQUOTED,
  SINGLE_QUOTED,
  DOUBLE_QUOTED = 0x3

} QuoteFlagE;

// TODO: DOCS
typedef enum RedirMode {
  REDIR_IN,
  REDIR_OUT = 01000,
  REDIR_APPEND = 02000
} RedirMode;

// TODO: DOCS
typedef struct Redirect {
   int fd;
  RedirMode mode;
  char* target;
} Redirect;

// TODO: DOCS
typedef struct Command {
  char** argv;
  size_t nredirs;
  Redirect redirs[];
} Command;

// TODO: DOCS
typedef struct WordNode {
  const char* value;
  struct WordNode* next;
} WordNode;

// TODO: DOCS
typedef struct WordList {
  size_t size;
  WordNode* head;
} WordList;

/**
 * @attention This takes ownership of head and its children.
 *
 */
// TODO: DOCS
WordList* new_from_nodes(WordNode* head);

// TODO: DOCS
Command* init_command(char** argv, size_t nredirs, Redirect* redirs);

// TODO: DOCS
Command* build_command(WordList* words);

// TODO: DOCS
void cleanup_command(Command* command);

/**
 *
 *
 * @param fd file descriptor
 * @param mode redirect mode
 * @param target redirection target
 * @return Redirection object
 */
// TODO: DOCS
Redirect* init_redirect(int fd, RedirMode mode, const char* target);

// TODO: DOCS
void cleanup_redirect(Redirect* redir);

// TODO: DOCS
WordNode* init_wordnode(const char* initial_word);

// TODO: DOCS
void cleanup_wordnode(WordNode* node);

// TODO: DOCS
WordList* empty_wordlist();

// TODO: DOCS
WordList* init_wordlist(const char* initial_word);

// TODO: DOCS
void cleanup_wordlist(WordList* list);

// TODO: DOCS
WordNode* append_wordlist(WordList* list, const char* word);

// TODO: DOCS
size_t next_token(char* dest, char* buf, QuoteFlagE* flag);

// TODO: DOCS
WordList* tokenize_input(char* buf);

// TODO: DOCS
WordList* tokenize_path(const char* path);

// TODO: DOCS
void prepare_args(char** dest, WordList* words);

// TODO: DOCS
void parse_redir(Redirect* dest, WordList* words);