//
// Created by nkinder on 9/3/26.
//

#include "TokenList.h"


size_t
tokenlist_count(TokenList* tokens) {
    Token* iter   = tokens->head;
    size_t    result = 0;
    while (iter != nullptr) {
        ++result;
        iter = iter->next;
    }
    return result;
}

TokenList*
tokenlist_from_tokens(Token* head) {
    TokenList* result = tokenlist_new_empty();
    if (!result) {
        return nullptr;
    }
    size_t    size = 0;
    Token* iter = head;
    while (iter != nullptr) {
        ++size;
        iter = iter->next;
    }

    result->size = size;
    result->head = head;

    return result;
}

Token*
token_new(TokenType type, const char* text, int fd) {
    char* word_copy = strdup(text);
    if (word_copy == nullptr) {
        return nullptr;
    }
    Token* result = malloc(sizeof(Token));
    if (result == nullptr) {
        free(word_copy);
        return nullptr;
    }
    result->type = type;
    result->value = word_copy;
    result->next  = nullptr;
    result->fd = fd;

    return result;
}

void
token_delete(Token* node) {
    free((void*) node->value);
    free(node);
}

TokenList*
tokenlist_new_empty() {
    TokenList* result = malloc(sizeof(TokenList));
    if (!result) {
        return nullptr;
    }

    result->head = nullptr;
    result->size = 0;
    return result;
}

TokenList*
tokenlist_new(TokenType type, const char* initial_word) {
    Token* head = newtok(type, initial_word);
    if (!head) {
        return nullptr;
    }

    TokenList* result = tokenlist_new_empty();
    if (!result) {
        token_delete(head);
        return nullptr;
    }

    result->size = 1;
    result->head = head;
    return result;
}

void
tokenlist_delete(TokenList* list) {
    Token* iter = list->head;
    Token* prev = nullptr;

    while (iter != nullptr) {
        prev = iter;
        iter = iter->next;
        token_delete(prev);
    }

    free(list);
}
size_t
token_count(Token* tok) {
  size_t chain = 0;
  if (tok) {
    chain++;
  }
  while (tok != nullptr) {
    tok = tok->next;
    ++chain;
  }
  return chain;
}

Token*
tokenlist_append(TokenList* list, TokenType type, const char* word) {
    Token* iter     = list->head;
    Token* new_node = newtok(type, word);
    if (list->head == nullptr) {
        list->head = new_node;
        list->size = 1;
        return new_node;
    }

    while (iter->next != nullptr) {
        iter = iter->next;
    }

    iter->next = new_node;
    new_node->type = type;
    list->size++;
    return new_node;
}
Token*
tokenlist_append_tok(TokenList* list, Token* token) {
  Token* iter = list->head;
  size_t sizeof_chain = token_count(token);
  if (iter == nullptr) {
    list->head = token;
    list->size += sizeof_chain;
  }

  while (iter->next != nullptr) {
    iter = iter->next;
  }

  iter->next = token;
  list->size += sizeof_chain;

}

TokenList*
tokenlist_copyof(TokenList* original) {
    Token* iter     = original->head;
    Token* new_head = token_new(iter->type, iter->value, iter->fd);
    Token* new_iter = new_head;
    iter               = iter->next;
    while (iter != nullptr) {
        new_iter->next = token_new(iter->type, iter->value, iter->fd);
        new_iter       = new_iter->next;
        iter           = iter->next;
    }
    TokenList* newlist = tokenlist_new_empty();
    newlist->size     = original->size;
    newlist->head     = new_head;
    return newlist;
}
