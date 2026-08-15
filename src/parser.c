//
// Created by nkinder on 8/13/26.
//
#include "parser.h"

#include <ctype.h>

#include "expand.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


void prepare_args(char** dest, WordList* words) {
  WordNode* iter = words->head;
  for (int i = 0; i < words->size; ++i) {
    memcpy(dest[i], iter->value, strlen(iter->value) + 1);
    iter = iter->next;
  }
  dest[words->size] = nullptr;
}

size_t argvlen(char** argv) {
  size_t len = 0;
  char** iter = argv;
  while (*iter != nullptr) {
    len++;
    iter++;
  }
  return ++len;
}

Command* init_command(char** argv, size_t nredirs, Redirect redirs[]) {
  size_t len = argvlen(argv);
  char** args = malloc(sizeof(char*) * len);

  for (int i = 0; i < len; ++i) {
    if (i == len - 1) {
      args[i] = nullptr;
      break;
    }
    char* tmp = strdup(argv[i]);
    if (!tmp) {
      for (int j = 0; j < i; ++j) {
        free(args[j]);
      }
      free(args);
      return nullptr;
    } else {
      args[i] = tmp;
    }
  }

  Command* command = malloc(sizeof(Command) + (sizeof(Redirect) * nredirs));
  if (!command) {
    for (int i = 0; i < len - 1; ++i) {
      free(args[i]);
    }
    free(args);
    return nullptr;
  }

  command->argv = args;
  command->nredirs = nredirs;
  memmove(command->redirs, redirs, sizeof(Redirect) * nredirs);


  return command;
}

WordList* new_from_nodes(WordNode* head) {
  WordList* result = empty_wordlist();
  if (!result) {
    return nullptr;
  }
  size_t size = 0;
  WordNode* iter = head;
  while (iter != nullptr) {
    ++size;
    iter = iter->next;
  }

  result->size = size;
  result->head = head;

  return result;
}

bool is_redir(const char* str) {
  const char* iter = str;
  while (*iter) {
    if (*iter == '>') {
      if (iter != str) {
        if (isdigit(*(iter - 1))) {
          return true;
        } else {
          return false;
        }
      } else {
        return true;
      }
    } else {
      ++iter;
    }
  }

  return false;
}

void parse_redir(Redirect* dest, WordList* words) {
  int fd = 1;
  RedirMode mode = REDIR_OUT;
  const char* iter = words->head->value;
  if (isdigit(*iter)) {
    const char* start = iter;
    do {
      ++iter;
    } while (isdigit(*iter));
    char* endptr;
    fd = (int)strtol(start, &endptr, 10);
  }
  if (*(iter + 1) == '>') {
    mode = REDIR_APPEND;
  } else if (*(iter + 1) == '\0') {
    mode = REDIR_OUT;
  }
  char* target = strdup(words->head->next->value);
  dest->fd = fd;
  dest->mode = mode;
  dest->target = target;

}

WordList* copy_wordlist(WordList* original) {
  WordNode* iter = original->head;
  WordNode* new_head = init_wordnode(iter->value);
  WordNode* new_iter = new_head;
  iter = iter->next;
  while (iter != nullptr) {
      new_iter->next = init_wordnode(iter->value);
      new_iter = new_iter->next;
      iter = iter->next;
  }
  WordList* newlist = empty_wordlist();
  newlist->size = original->size;
  newlist->head = new_head;
  return newlist;
}

Command* build_command(WordList* words) {
  char argbuf[50][50];
  char* argdest[50];
  for (int i = 0; i < 50; ++i) {
    argdest[i] = argbuf[i];
  }
  Redirect redirs [5];
  WordList* wordcopy = copy_wordlist(words);

  size_t nredirs = 0;

  WordNode* iter = wordcopy->head;
  while (iter != nullptr) {
    if (is_redir(iter->value)) {
      WordList* redirected = new_from_nodes(iter);
      wordcopy->size = wordcopy->size - redirected->size;
      parse_redir(&redirs[nredirs++], redirected);
    }
    iter = iter->next;
  }
  prepare_args(argdest, wordcopy);
  Command* result = init_command(argdest, nredirs, redirs);
  if (!result) {
    for (int i = 0; i < nredirs; ++i) {
      free(redirs[i].target);
    }
    cleanup_wordlist(wordcopy);
    return nullptr;
  }
  return result;
}

void cleanup_command(Command* command) {
  char** iter = command->argv;
  while (*iter != nullptr) {
    free(*iter++);
  }
  free(command->argv);
  free(command);

}

Redirect* init_redirect(int fd, RedirMode mode, const char* target) {
  Redirect* result = malloc(sizeof(Redirect));
  if (!result) {
    return nullptr;
  }

  char* redirect_target = strdup(target);
  if (!redirect_target) {
    free(result);
    return nullptr;
  }

  result->fd = fd;
  result->mode = mode;
  result->target = redirect_target;

  return result;
}

void cleanup_redirect(Redirect* redir) {
  free(redir->target) ;
  free(redir);
}

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
  free((void*)node->value);
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
    free((void*)s);
    iter = iter->next;
  }
  return 0;
}

WordList *tokenize_input(const char *input) {
  QuoteFlagE flag = UNQUOTED;
  char buf[1024] = {};
  const char* iter = input;
  WordList* result = empty_wordlist();

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

  const char* end = path + strlen(path);
  const char* iter = path;
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