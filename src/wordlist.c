//
// Created by nkinder on 9/5/26.
//
// Description: This is basically a copy of the TokenList that I refactored out.
//              I forgot that some other parts of the codebase utilized it and
//              the "TokenType" field is not applicable.
#include "wordlist.h"
#include "common.h"

Word*
wordlist_append_word(WordList* list, Word* node) {
    Word* iter = list->head;
    if (iter == nullptr) {
      list->head = node;
      list->size++;
      return list->head;
    }
    while (iter->next != nullptr) {
        iter = iter->next;
    }
    iter->next = node;
    list->size++;
    return node;
}

size_t
wordlist_count(WordList* words) {
    Word*  iter  = words->head;
    size_t count = 0;
    while (iter != nullptr) {
        ++count;
        iter = iter->next;
    }

    return count;
}

WordList*
wordlist_from_words(Word* head) {
    WordList* lst = malloc(sizeof(WordList));
    if (!lst) {
        return nullptr;
    }

    lst->head = head;
    lst->size = wordlist_count(lst);
    return lst;
}

inline Word*
word_new(const char* text) {
    Word* word = malloc(sizeof(WordList));
    if (!word) {
        return nullptr;
    }

    const char* text_copy = strdup(text);
    if (!text_copy) {
        free(word);
        return nullptr;
    }

    word->text = text_copy;
    word->next = nullptr;
    return word;
}

Word*
word_copyof(Word* original) {
    Word* result = word_new(original->text);
    result->next = nullptr;
    return result;
}

void
word_delete(Word* word) {
    free(word->text);
    free(word);
}

WordList*
wordlist_new_empty() {
    WordList* lst = malloc(sizeof(WordList));
    if (!lst) {
        return lst;
    }
    lst->size = 0;
    lst->head = nullptr;
    return lst;
}
WordList*
wordlist_new(const char* text) {
    WordList* lst = wordlist_new_empty();
    if (!lst) {
        return lst;
    }

    wordlist_append(lst, text);

    return lst;
}

WordList*
wordlist_copyof(WordList* list) {
    WordList* lst  = wordlist_new_empty();
    Word*     iter = list->head;
    while (iter != nullptr) {
        Word* copy = word_copyof(iter);
        wordlist_append_word(lst, copy);
        iter = iter->next;
    }

    return lst;
}

void
wordlist_delete(WordList* list) {
    Word* iter = list->head;
    while (iter != nullptr) {
        Word* tmp = iter;
        iter      = iter->next;
        word_delete(tmp);
    }
    free(list);
}

Word*
wordlist_append(WordList* list, const char* word) {
    Word* new_node = word_new(word);
    wordlist_append_word(list, new_node);
    return new_node;
}
