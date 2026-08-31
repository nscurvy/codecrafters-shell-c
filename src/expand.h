//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "nullability.h"

extern struct HashTable* variable_table;

ASSUME_NONNULL_BEGIN
/**
 * Perform expansion on a token word.
 * Allocates space for the expanded string.
 * @param word Null terminated string
 * @return a new heap allocated string with expansions performed.
 */
char * NULLABLE exptok(const char* NULLABLE word)
GCC_NONNULL(1);

// TODO: DOCS
char* NULLABLE expand_home(char* NONNULL buf, const char* NONNULL path)
GCC_NONNULL(1, 2);

ASSUME_NONNULL_END