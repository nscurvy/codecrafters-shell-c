//
// Created by nkinder on 9/3/26.
//

#pragma once
#include "common.h"
#include "nullability.h"

/**
 * @brief A single node in a singly linked list of tokenized words.
 */
typedef struct WordNode {
    const char*               value; /**< Heap-allocated token text owned by this node. */
    struct WordNode* NULLABLE next;  /**< Next node in the list, or @c nullptr if last. */
} WordNode;

/**
 * @brief A singly linked list of tokens, with an explicit element count.
 */
typedef struct WordList {
    size_t             size; /**< Number of nodes reachable from #head. */
    WordNode* NULLABLE head; /**< First node in the list, or @c nullptr if empty. */
} WordList;

ASSUME_NONNULL_BEGIN

size_t
count_words(WordList* tokens) GCC_NONNULL(1);

/**
 * @brief Wrap an existing chain of word nodes in a new WordList.
 *
 * Walks the chain starting at @p head to compute its length, then builds a
 * new WordList whose @c head points directly at @p head (the nodes
 * themselves are not copied).
 *
 * @attention This takes ownership of head and its children.
 *
 * @param head First node of an existing (possibly detached) chain of
 *             WordNode objects.
 *
 * @return A newly allocated WordList taking ownership of @p head, or
 *         @c nullptr if allocation of the list itself fails.
 */
WordList* NULLABLE
new_from_nodes(WordNode* head) GCC_NONNULL(1);

/**
 * @brief Allocate a WordNode owning a copy of the given string.
 *
 * @param initial_word String to duplicate into the new node.
 *
 * @return A newly allocated WordNode with @c next set to @c nullptr, or
 *         @c nullptr if allocation or duplication failed.
 */
WordNode* NULLABLE
init_wordnode(const char* initial_word) GCC_NONNULL(1);

/**
 * @brief Free a single WordNode and its owned value string.
 *
 * @param node Node to free. Does not touch @c node->next; use
 *             cleanup_wordlist() to free an entire chain.
 */
void
cleanup_wordnode(WordNode* node) GCC_NONNULL(1);

/**
 * @brief Allocate an empty WordList.
 *
 * @return A newly allocated WordList with @c size 0 and @c head
 *         @c nullptr, or @c nullptr on allocation failure.
 */
WordList* NULLABLE
empty_wordlist();

/**
 * @brief Allocate a WordList containing a single word.
 *
 * @param initial_word String to duplicate as the list's first element.
 *
 * @return A newly allocated WordList of size 1, or @c nullptr if any
 *         allocation failed.
 */
WordList* NULLABLE
init_wordlist(const char* initial_word) GCC_NONNULL(1);

WordList*
copy_wordlist(WordList* list);

/**
 * @brief Free a WordList and every node it contains.
 *
 * @param list List to free, including all of its WordNode elements and
 *             their owned value strings.
 */
void
cleanup_wordlist(WordList* list) GCC_NONNULL(1);

/**
 * @brief Append a new word to the end of a WordList.
 *
 * @param list List to append to. If empty, the new node becomes the head.
 * @param word String to duplicate into the newly appended node.
 *
 * @return The newly appended WordNode, or @c nullptr if allocation failed.
 */
WordNode* NULLABLE
append_wordlist(WordList* list, const char* word) GCC_NONNULL(1, 2);


ASSUME_NONNULL_END
