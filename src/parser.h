//
// Created by nkinder on 8/13/26.
//

#pragma once
#include <stdlib.h>
#include <linux/limits.h>

#define MAX_REDIRS 1;

typedef enum QuoteFlagE {
  UNQUOTED,
  SINGLE_QUOTED,
  DOUBLE_QUOTED = 0x3

} QuoteFlagE;

typedef enum RedirMode {
  REDIR_IN,
  REDIR_OUT = 01000,
  REDIR_APPEND = 02000
} RedirMode;

typedef struct Redirect {
   int fd;
  RedirMode mode;
  char* target;
} Redirect;

typedef struct Command {
  char** argv;
  size_t nredirs;
  Redirect redirs[];
} Command;

typedef struct WordNode {
  const char* value;
  struct WordNode* next;
} WordNode;

typedef struct WordList {
  size_t size;
  WordNode* head;
} WordList;

/**
 * @attention This takes ownership of head and its children.
 *
 */
WordList* new_from_nodes(WordNode* head);

Command* init_command(char** argv, size_t nredirs, Redirect* redirs);

Command* build_command(WordList* words);

void cleanup_command(Command* command);

/**
 *
 *
 * @param fd file descriptor
 * @param mode redirect mode
 * @param target redirection target
 * @return Redirection object
 */
Redirect* init_redirect(int fd, RedirMode mode, const char* target);

void cleanup_redirect(Redirect* redir);

WordNode* init_wordnode(const char* initial_word);

void cleanup_wordnode(WordNode* node);

WordList* empty_wordlist();

WordList* init_wordlist(const char* initial_word);

void cleanup_wordlist(WordList* list);

WordNode* append_wordlist(WordList* list, const char* word);

size_t next_token(char* dest, char* buf, QuoteFlagE* flag);

WordList* tokenize_input(char* buf);

WordList* tokenize_path(const char* path);

void prepare_args(char** dest, WordList* words);

void parse_redir(Redirect* dest, WordList* words);