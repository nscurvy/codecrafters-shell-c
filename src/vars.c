//
// Created by nkinder on 9/14/26.
//

#include "vars.h"
#include "common.h"
#include "hashtable.h"
#include "stringbuilder.h"

static struct HashTable* variable_table = nullptr;
extern char**            environ;
void
assignment_split(char* dest[2], const char* assignment);

void
var_table_check() {
    if (variable_table == nullptr) {
        variable_table = ht_new();
    }
}

const char*
var_lookup(const char* name) {
    var_table_check();
    return ht_get(variable_table, name);
}

void
var_assign(const char* name, const char* value) {
    var_table_check();
    ht_put(variable_table, name, value, false);
}

void
var_export(const char* name, const char* value) {
    var_table_check();
    ht_put(variable_table, name, value, true);
}

bool
var_unassign(const char* name) {
    var_table_check();
    bool result = false;
    if (ht_contains(variable_table, name)) {
        ht_remove(variable_table, name);
        result = true;
    }
    return result;
}


void
var_init_from_environ() {
    var_table_check();
    const char** ei = (const char**) environ;
    char*        buf[2];
    while (*ei != nullptr) {
        assignment_split(buf, *ei);
        ht_put(variable_table, buf[0], buf[1], true);
        ++ei;
        free(buf[0]);
        free(buf[1]);
    }
}

void
fork_exported() {
    var_table_check();
    struct HashTable* var_copy = ht_new();
    struct HashTable* var_old  = variable_table;
    ItemList*         old_set  = ht_entryset(var_old);
    while (old_set->head != nullptr) {
        ItemNode* node = il_pull(old_set);
        if (node->exported) {
            ht_put(var_copy, node->key, node->value, node->exported);
            in_delete(node);
        } else {
            in_delete(node);
        }
    }
    ht_delete(var_old);
    variable_table = var_copy;
}


const char*
var_lookupn(const char* name, size_t count) {
    var_table_check();
    return ht_getn(variable_table, name, count);
}
