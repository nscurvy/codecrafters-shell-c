//
// Created by nkinder on 9/5/26.
//
// Description: This is basically a copy of the TokenList that I refactored out.
//              I forgot that some other parts of the codebase utilized it and
//              the "TokenType" field is not applicable.

#pragma once
#include <stddef.h>
#include "nullability.h"

typedef struct Word {
  const char* text;
  struct Word* next;
} Word;

typedef struct WordList {
  size_t size;
  Word* head;

} WordList;
ASSUME_NONNULL_BEGIN

size_t
wordlist_count(WordList* words) GCC_NONNULL(1);

WordList* NULLABLE
wordlist_from_words(Word* head) GCC_NONNULL(1);

Word* NULLABLE
word_new(const char* text) GCC_NONNULL(1);

void
word_delete(Word* word) GCC_NONNULL(1);

WordList* NULLABLE
wordlist_new_empty();

WordList* NULLABLE
wordlist_new(const char* text) GCC_NONNULL(1);

WordList*
wordlist_copyof(WordList* list);

void
wordlist_delete(WordList* list) GCC_NONNULL(1);

Word* NULLABLE
wordlist_append(WordList* list, const char* word) GCC_NONNULL(1, 2);

ASSUME_NONNULL_END
