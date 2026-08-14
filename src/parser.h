//
// Created by nkinder on 8/13/26.
//

#pragma once
#include <stdlib.h>

typedef enum QuoteFlagE {
  UNQUOTED,
  SINGLE_QUOTED,
  DOUBLE_QUOTED = 0x3

} QuoteFlagE;

typedef struct WordNode {
  const char* value;
  struct WordNode* next;
} WordNode;

typedef struct WordList {
  size_t size;
  WordNode* head;
} WordList;

WordNode* init_wordnode(const char* initial_word);

void cleanup_wordnode(WordNode* node);

WordList* empty_wordlist();

WordList* init_wordlist(const char* initial_word);

void cleanup_wordlist(WordList* list);

WordNode* append_wordlist(WordList* list, const char* word);

int next_token(char* dest, char* buf, QuoteFlagE* flag);

WordList* tokenize_input(char* buf);

WordList* tokenize_path(const char* path);