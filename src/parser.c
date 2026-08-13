//
// Created by nkinder on 8/13/26.
//
#include "parser.h"

#include <string.h>

WordNode *init_wordnode(const char *initial_word) {
  char* word_copy = strdup(initial_word);
  if (word_copy == nullptr) {
    return nullptr;
  }
  WordNode* result = malloc(sizeof(WordNode));
  if (result == nullptr) {
    free(word_copy);
    return nullptr;
  }

  result->value = word_copy;
  result->next = nullptr;

  return result;
}

void cleanup_wordnode(WordNode *node) {
  free(node->value);
  free(node);
}

WordList *empty_wordlist() {
  WordList* result = malloc(sizeof(WordList));
  if (!result) {
    return nullptr;
  }

  result->head = nullptr;
  result->size = 0;
  return result;
}

WordList *init_wordlist(const char *initial_word) {
  WordNode* head = init_wordnode(initial_word);
  if (!head) {
    return nullptr;
  }

  WordList* result = empty_wordlist();
  if (!result) {
    cleanup_wordnode(head);
    return nullptr;
  }

  result->size = 1;
  result->head = head;
  return result;
}

void cleanup_wordlist(WordList *list) {
  WordNode* iter = list->head;
  WordNode* prev = nullptr;

  while (iter != nullptr) {
    prev = iter;
    iter = iter->next;
    cleanup_wordnode(prev);
  }

  free(list);
}

WordNode *append_wordlist(WordList *list, const char *word) {
  WordNode* iter = list->head;
  WordNode* new_node = init_wordnode(word);
  if (list->head == nullptr) {
    list->head = new_node;
    list->size = 1;
    return new_node;
  }

  while (iter->next != nullptr) {
    iter = iter->next;
  }

  iter->next = new_node;
  list->size++;
  return new_node;
}

int next_token(char* dest, char *input) {
  char* i = input;
  int count = 0;
  while (*i != '\0' && *i != ' ' && *i != '\n' && count < 1024 - 1) {
    dest[count] = *i;
    ++i;
    ++count;
  }
  if (count == 0) {
    return 0;
  }

  dest[count] = '\0';
  return  count;
}

WordList *tokenize_input(char *input) {
  char buf[1024] = {};
  char* iter = input;
  WordList* result = empty_wordlist();

  char* token = nullptr;
  int readchars;

  do {

    readchars = next_token(buf, iter);
    if (readchars > 0) {
      size_t jumpsize = strlen(buf);
      iter += jumpsize + 1;
      append_wordlist(result, buf);
    }

  } while (readchars != 0);

  return result;
}
