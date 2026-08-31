//
// Created by nkinder on 8/30/26.
//

#pragma once
#include <stddef.h>

struct HashTable;


struct HashTable* init_ht();

void cleanup_ht(struct HashTable* table);

int ht_put(struct HashTable* table, const char* key, const char* value);

const char* ht_get(struct HashTable* table, const char* key);

int ht_remove(struct HashTable* table, const char* key);

int ht_clear(struct HashTable* table);

bool ht_contains(struct HashTable* table, const char* key);

size_t ht_size(struct HashTable* table);

