//
// Created by nkinder on 8/13/26.
//
#include "parser.h"
#include "expand.h"

#include <stddef.h>
#include <stdio.h>
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

size_t next_token(char* dest, char *input, QuoteFlagE* flag) {
  char* i = input;
  int destidx= 0;


  while (*i) {
    char c = *i;
    if (*flag == UNQUOTED || *flag == DOUBLE_QUOTED) {

      if (c == '\\') {
        dest[destidx++] = *i++;
        if (*i == '\0') {
          break;
        }
        if (*flag == UNQUOTED || strchr("\\$`\"\n", *i)) {
          dest[destidx++] = *i++;
          continue;
        }
        dest[destidx++] = '\\';
        continue;
      }
    }
    if (c == '\'') {
      switch (*flag) {
      case SINGLE_QUOTED:
          *flag = UNQUOTED;
          dest[destidx++] = c;
        break;

      case UNQUOTED:
          *flag = SINGLE_QUOTED;
          dest[destidx++] = c;
        break;

      case DOUBLE_QUOTED:
        dest[destidx++] = c;
        break;

      }
      ++i;
    } else if (c == '"') {
      switch (*flag) {
      case SINGLE_QUOTED:
        dest[destidx++] = c;
        break;

      case UNQUOTED:
        *flag = DOUBLE_QUOTED;
        dest[destidx++] = c;
        break;

      case DOUBLE_QUOTED:
        *flag = UNQUOTED;
        dest[destidx++] = c;
        break;

      }
      ++i;
    }else if (*flag == UNQUOTED && (c == ' ' || c == '\n')) {
      if (c == ' ') {
        do {
          //++destidx;
          ++i;
        } while (*i == ' ');
      }
      break;
    } else {
      dest[destidx++] = c;
      ++i;
    }
  }

  //while (*i != '\0' && *i != ' ' && *i != '\n' && count < 1024 - 1) {
  //  dest[count] = *i;
  //  ++i;
  //  ++count;
  //}
  if (destidx == 0) {
    return 0;
  }

  dest[destidx] = '\0';
  return (size_t)(i - input);
}

int exppass(WordList* list) {
  WordNode* iter = list->head;

  for (int i = 0; i < list->size; ++i) {
    const char* s = iter->value;
    const char* expanded = exptok(s);
    iter->value = expanded;
    free(s);
    iter = iter->next;
  }
  return 0;
}

WordList *tokenize_input(char *input) {
  QuoteFlagE flag = UNQUOTED;
  char buf[1024] = {};
  char* iter = input;
  WordList* result = empty_wordlist();

  char* token = nullptr;
  size_t readchars;

  do {

    readchars = next_token(buf, iter, &flag);
    if (readchars > 0) {
      size_t jumpsize = readchars;
      iter += jumpsize;
      append_wordlist(result, buf);
      memset(buf, 0, readchars);
    }

  } while (readchars != 0);
  if (flag == SINGLE_QUOTED) {
    cleanup_wordlist(result);
    fprintf(stderr, "syntax error: unterminated quote\n");
    setbuf(stderr, nullptr);
    return nullptr;
  }

  exppass(result);

  return result;
}

WordList* tokenize_path(const char* path) {
  WordList* result = empty_wordlist();
  char* tok = calloc((strlen(path) + 1),  sizeof(char));

  char* end = path + strlen(path);
  char* iter = path;
  ptrdiff_t bytesremaining = end - path;
  while (!(iter >= end)) {
    memset(tok, '\0', strlen(tok));
    memccpy(tok, iter, ':', bytesremaining);
    tok[strcspn(tok, ":")] = '\0';
    append_wordlist(result, tok);
    bytesremaining = end - iter;
    iter = iter + strlen(tok) + 1;
  }

  return result;
}