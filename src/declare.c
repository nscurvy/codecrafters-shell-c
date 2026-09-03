//
// Created by nkinder on 8/30/26.
//

#include "declare.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
static const uint64_t FNV_PRIME        = 0x100000001b3;

static const size_t DEFAULT_INITIAL_CAPACITY = 16;
static const size_t MAX_CAPACITY             = 1 << 30;

static const float DEFAULT_LOAD_FACTOR     = 0.75f;
static const float DEFAULT_MAX_LOAD_FACTOR = 0.9f;


typedef struct BucketNode {
    const char*        key;
    const char*        value;
    struct BucketNode* next;
} BucketNode;

BucketNode*
init_bucketnode(const char* name, const char* value) {
    BucketNode* node = malloc(sizeof(BucketNode));
    if (node == nullptr) {
        return nullptr;
    }
    node->key   = strdup(name);
    node->value = strdup(value);
    node->next  = nullptr;
    return node;
}

void
cleanup_bucketnode(BucketNode* node) {
    free(node->key);
    free(node->value);
    free(node);
}

typedef struct BucketList {
    size_t      size;
    BucketNode* head;
} BucketList;


BucketList*
init_bucketlist() {
    BucketList* list = malloc(sizeof(BucketList));
    if (list == nullptr) {
        return nullptr;
    }
    list->size = 0;
    list->head = nullptr;
    return list;
}

void
cleanup_bucketlist(BucketList* bucket_list) {
    BucketNode* iter = bucket_list->head;
    while (iter != nullptr) {
        BucketNode* tmp = iter;
        iter            = iter->next;
        cleanup_bucketnode(tmp);
    }
    free(bucket_list);
}

bool
bucketlist_append(BucketList* bucket_list, const char* name, const char* value) {
    BucketNode* node = init_bucketnode(name, value);
    if (node == nullptr) {
        return false;
    }
    if (bucket_list->size == 0) {
        bucket_list->head = node;
        ++bucket_list->size;
        return true;
    }

    BucketNode* iter = bucket_list->head;
    while (iter->next != nullptr) {
        iter = iter->next;
    }
    iter->next = node;
    ++bucket_list->size;
    return true;
}

const char*
bucketlist_get(BucketList* bucket_list, const char* name) {
    BucketNode* iter = bucket_list->head;
    while (iter != nullptr) {
        if (strcmp(iter->key, name) == 0) {
            return iter->value;
        }
        iter = iter->next;
    }
    return nullptr;
}

bool
bucketlist_remove(BucketList* bucket_list, const char* name) {
    bool result = false;

    BucketNode* iter = bucket_list->head;
    BucketNode* prev = nullptr;
    while (iter != nullptr) {
        if (strcmp(iter->key, name) == 0) {
            result     = true;
            prev->next = iter->next;
            cleanup_bucketnode(iter);
            --bucket_list->size;
        }
        prev = iter;
        iter = iter->next;
    }

    return result;
}

uint64_t
fnv_1a(const char* str) {
    uint64_t hash = FNV_OFFSET_BASIS;
    while (*str != '\0') {
        hash ^= (uint8_t) *str;
        hash *= FNV_PRIME;
        ++str;
    }

    return hash;
}

typedef struct HashTable {
    float        load_factor;
    size_t       size;
    size_t       capacity;
    BucketList** buckets;
} HashTable;

struct HashTable*
init_ht() {
    errno                   = 0;
    struct HashTable* table = malloc(sizeof(HashTable));
    if (table == nullptr) {
        fprintf(stderr, "Couldn't allocate variable table: %s\n", strerror(errno));
        exit(errno);
    }
    table->load_factor = DEFAULT_LOAD_FACTOR;
    table->capacity    = DEFAULT_INITIAL_CAPACITY;
    errno              = 0;
    table->buckets     = calloc(table->capacity, sizeof(BucketList*));
    if (table->buckets == nullptr) {
        free(table);
        fprintf(stderr, "Couldn't allocate variable table: %s\n", strerror(errno));
        exit(errno);
    }
    table->size = 0;
    for (int i = 0; i < table->capacity; ++i) {
        errno             = 0;
        table->buckets[i] = init_bucketlist();
        if (table->buckets[i] == nullptr) {
            for (int j = 0; j < i; ++j) {
                cleanup_bucketlist(table->buckets[j]);
            }
            free(table->buckets);
            free(table);
            fprintf(stderr, "Couldn't allocate variable table: %s\n", strerror(errno));
            exit(errno);
        }
    }

    return table;
}

void
cleanup_ht(HashTable* table) {
    for (int i = 0; i < table->capacity; ++i) {
        cleanup_bucketlist(table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}

int
hashtable_resize(HashTable* table, size_t new_capacity) {
    bool succ = 0;

    if (new_capacity >= MAX_CAPACITY) {
        errno = EINTR;
        succ  = -1;
    } else {
        BucketList** tmp = calloc(new_capacity, sizeof(BucketList*));
        if (tmp == nullptr) {
            errno = ENOMEM;
            succ  = -1;
        } else {
            BucketList** old_buckets = table->buckets;
            table->buckets           = tmp;
            size_t oldcap            = table->capacity;
            table->capacity          = new_capacity;
            for (size_t i = 0; i < new_capacity; ++i) {
                tmp[i] = init_bucketlist();
                if (!tmp[i]) {
                    errno = ENOMEM;
                    succ  = -1;
                    break;
                }
            }
            for (size_t i = 0; i < oldcap; ++i) {
                BucketNode* iter = old_buckets[i]->head;
                while (iter != nullptr) {
                    ht_put(table, iter->key, iter->value);
                    iter = iter->next;
                }
                cleanup_bucketlist(old_buckets[i]);
            }
            free(old_buckets);
        }
    }

    return succ;
}

int
ht_put(HashTable* table, const char* key, const char* value) {
    int      added = -1;
    uint64_t hash  = fnv_1a(key);
    hash %= table->capacity;
    if (table->size >= table->capacity * table->load_factor) {
        errno = 0;
        if (hashtable_resize(table, table->capacity << 1)) {
            fprintf(stderr, "Error resizing hash table: %s\n", strerror(errno));
            return -1;
        }
    }
    BucketNode* iter      = table->buckets[hash]->head;
    BucketNode* prev      = nullptr;
    bool        is_unique = true;
    while (iter != nullptr) {
        if (strcmp(iter->key, key) == 0) {
            free(iter->value);
            iter->value = strdup(value);
            break;
        }
        prev = iter;
        iter = iter->next;
    }

    if (is_unique) {
        // The case where the head is a nullptr
        if (prev == nullptr) {
            table->buckets[hash]->head = init_bucketnode(key, value);
            table->size++;
        } else {
            prev->next = init_bucketnode(key, value);
            table->size++;
        }
    }
    return added;
}

bool
ht_contains(HashTable* table, const char* key) {
    const char* result = ht_get(table, key);
    return result != nullptr;
}

const char*
ht_get(struct HashTable* table, const char* key) {
    uint64_t hash = fnv_1a(key);
    hash %= table->capacity;
    BucketNode* iter = table->buckets[hash]->head;
    while (iter != nullptr) {
        if (strcmp(iter->key, key) == 0) {
            return iter->value;
        }
        iter = iter->next;
    }
    return nullptr;
}

int
ht_remove(struct HashTable* table, const char* key) {
    uint64_t hash = fnv_1a(key);
    hash %= table->capacity;

    const char* member = bucketlist_get(table->buckets[hash], key);
    if (!member) {
        return -1;
    }
    bucketlist_remove(table->buckets[hash], key);
    table->size--;
    return 0;
}

int
ht_clear(struct HashTable* table) {
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->buckets[i]->size > 0) {
            BucketNode* node = table->buckets[i]->head;
            while (node != nullptr) {
                BucketNode* tmp = node;
                node            = node->next;
                cleanup_bucketnode(tmp);
            }
            table->size -= table->buckets[i]->size;
            table->buckets[i]->size = 0;
        }
    }
    return 0;
}


size_t
ht_size(struct HashTable* table) {
    return table->size;
}
