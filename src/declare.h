//
// Created by nkinder on 8/30/26.
//

#pragma once
#include <stddef.h>
#include "nullability.h"

ASSUME_NONNULL_BEGIN

struct HashTable;


struct HashTable*
init_ht();

void
cleanup_ht(struct HashTable* table) GCC_NONNULL(1);

int
ht_put(struct HashTable* table, const char* key, const char* value) GCC_NONNULL(1, 2, 3);

const char*
ht_get(struct HashTable* table, const char* key) GCC_NONNULL(1, 2);

int
ht_remove(struct HashTable* table, const char* key) GCC_NONNULL(1);

int
ht_clear(struct HashTable* table) GCC_NONNULL(1);

bool
ht_contains(struct HashTable* table, const char* key) GCC_NONNULL(1, 2);

size_t
ht_size(struct HashTable* table);

ASSUME_NONNULL_END
