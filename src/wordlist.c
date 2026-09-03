//
// Created by nkinder on 9/3/26.
//

#include "wordlist.h"


size_t
count_words(WordList* tokens) {
    WordNode* iter   = tokens->head;
    size_t    result = 0;
    while (iter != nullptr) {
        ++result;
        iter = iter->next;
    }
    return result;
}

WordList*
new_from_nodes(WordNode* head) {
    WordList* result = empty_wordlist();
    if (!result) {
        return nullptr;
    }
    size_t    size = 0;
    WordNode* iter = head;
    while (iter != nullptr) {
        ++size;
        iter = iter->next;
    }

    result->size = size;
    result->head = head;

    return result;
}

WordNode*
init_wordnode(const char* initial_word) {
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
    result->next  = nullptr;

    return result;
}

void
cleanup_wordnode(WordNode* node) {
    free((void*) node->value);
    free(node);
}

WordList*
empty_wordlist() {
    WordList* result = malloc(sizeof(WordList));
    if (!result) {
        return nullptr;
    }

    result->head = nullptr;
    result->size = 0;
    return result;
}

WordList*
init_wordlist(const char* initial_word) {
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

void
cleanup_wordlist(WordList* list) {
    WordNode* iter = list->head;
    WordNode* prev = nullptr;

    while (iter != nullptr) {
        prev = iter;
        iter = iter->next;
        cleanup_wordnode(prev);
    }

    free(list);
}

WordNode*
append_wordlist(WordList* list, const char* word) {
    WordNode* iter     = list->head;
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

WordList*
copy_wordlist(WordList* original) {
    WordNode* iter     = original->head;
    WordNode* new_head = init_wordnode(iter->value);
    WordNode* new_iter = new_head;
    iter               = iter->next;
    while (iter != nullptr) {
        new_iter->next = init_wordnode(iter->value);
        new_iter       = new_iter->next;
        iter           = iter->next;
    }
    WordList* newlist = empty_wordlist();
    newlist->size     = original->size;
    newlist->head     = new_head;
    return newlist;
}
