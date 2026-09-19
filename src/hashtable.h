//
// Created by nkinder on 8/30/26.
//

#pragma once
#include "nullability.h"
#include <stddef.h>

ASSUME_NONNULL_BEGIN

struct HashTable;
typedef struct ItemNode {
    const char*      key;
    const char*      value;
    struct ItemNode* next;
    bool             exported;
} ItemNode;
typedef struct ItemList {
    size_t    size;
    ItemNode* head;
} ItemList;
struct BucketNode;

ItemNode*
in_new(const char* key, const char* value, bool exported);

void
in_delete(ItemNode* item);


ItemNode*
from_node(struct BucketNode* node);

ItemList*
il_new();

ItemNode*
il_append(ItemList* list, const char* key, const char* value, bool exported);

ItemNode*
il_append_node(ItemList* list, ItemNode* node);

ItemNode*
il_pull(ItemList* list);

void
il_delete(ItemList* list);


struct HashTable*
ht_new();

void
ht_delete(struct HashTable* table) GCC_NONNULL(1);

int
ht_put(struct HashTable* table, const char* key, const char* value, bool exported) GCC_NONNULL(1, 2, 3);

const char*
ht_get(struct HashTable* table, const char* key) GCC_NONNULL(1, 2);

const char*
ht_getn(struct HashTable* table, const char* key, size_t count);

int
ht_remove(struct HashTable* table, const char* key) GCC_NONNULL(1);

int
ht_clear(struct HashTable* table) GCC_NONNULL(1);

bool
ht_contains(struct HashTable* table, const char* key) GCC_NONNULL(1, 2);

size_t
ht_size(struct HashTable* table);

int
ht_get_exported(struct HashTable* table, bool* dst, const char* key);

ItemList*
ht_entryset(struct HashTable* table);

ASSUME_NONNULL_END
