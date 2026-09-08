//
// Created by nkinder on 9/3/26.
//

#pragma once
#include "common.h"
#include "nullability.h"


typedef enum TokenType {
    TOK_WORD,            // Normal text
    TOK_ASSIGNMENT_WORD, // NAME=value
    TOK_PIPE,            // |
    TOK_AND,             // &&
    TOK_OR,              // ||
    TOK_SEMI,            // ; TODO: Impl
    TOK_AMP,             // & TODO: Impl
    TOK_LPAREN,          // ( TODO: Impl
    TOK_RPAREN,          // ) TODO: Impl
    TOK_LBRACE,          // { TODO: Impl
    TOK_RBRACE,          // } TODO: Impl
    TOK_REDIR_IN,        // <
    TOK_REDIR_OUT,       // >
    TOK_REDIR_APPEND,    // >>
    TOK_REDIR_HEREDOC,   //
    TOK_NEWLINE,         // \n
    TOK_RESERVED_WORD,   // if/then/else/fi/while/do/done/for/case/esac
    TOK_CMDSUB_START,    // $( or `
    TOK_CMDSUB_END,      // ) or `
    TOK_EOF = -1,
    TOK_ERR = -2
} TokenType;


/**
 * @brief A single node in a singly linked list of tokenized words.
 */
typedef struct Token {
    TokenType              type;  /**< The type of token this is. */
    const char*            value; /**< Heap-allocated token text owned by this node. */
    int                    fd;    /**< Which file descriptor the token refers to. Only meaningful for redirects.*/
    struct Token* NULLABLE next;  /**< Next node in the list, or @c nullptr if last. */
} Token;

/**
 * @brief A singly linked list of tokens, with an explicit element count.
 */
typedef struct TokenList {
    size_t          size; /**< Number of nodes reachable from #head. */
    Token* NULLABLE head; /**< First node in the list, or @c nullptr if empty. */
} TokenList;

ASSUME_NONNULL_BEGIN

size_t
tokenlist_count(TokenList* tokens) GCC_NONNULL(1);

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
TokenList* NULLABLE
tokenlist_from_tokens(Token* head) GCC_NONNULL(1);

/**
 * @brief Allocate a WordNode owning a copy of the given string.
 *
 * @param text String to duplicate into the new node.
 *
 * @return A newly allocated WordNode with @c next set to @c nullptr, or
 *         @c nullptr if allocation or duplication failed.
 */
Token* NULLABLE
token_new(TokenType type, const char* text, int fd) GCC_NONNULL(1);

/**
 * @brief Convenience macro/overload to create new tokens in the vast majority of cases where an fd argument is
 * meaningless.
 * \see \link token_new \endlink
 *
 * @param type The type of new token. \link(Token#type)
 * @param text The text of the token. \link(Token#text)
 */
#define newtok(type, text) token_new((type), (text), 0)

/**
 * @brief Free a single WordNode and its owned value string.
 *
 * @param node Node to free. Does not touch @c node->next; use
 *             cleanup_wordlist() to free an entire chain.
 */
void
token_delete(Token* node) GCC_NONNULL(1);

/**
 * @brief Allocate an empty WordList.
 *
 * @return A newly allocated WordList with @c size 0 and @c head
 *         @c nullptr, or @c nullptr on allocation failure.
 */
TokenList* NULLABLE
tokenlist_new_empty();

/**
 * @brief Allocate a WordList containing a single word.
 *
 * @param initial_word String to duplicate as the list's first element.
 *
 * @return A newly allocated WordList of size 1, or @c nullptr if any
 *         allocation failed.
 */
TokenList* NULLABLE
tokenlist_new(TokenType type, const char* initial_word) GCC_NONNULL(1);

TokenList*
tokenlist_copyof(TokenList* list);

/**
 * @brief Free a WordList and every node it contains.
 *
 * @param list List to free, including all of its WordNode elements and
 *             their owned value strings.
 */
void
tokenlist_delete(TokenList* list) GCC_NONNULL(1);

/**
 * @brief Append a new word to the end of a WordList.
 *
 * @param list List to append to. If empty, the new node becomes the head.
 * @param word String to duplicate into the newly appended node.
 *
 * @return The newly appended WordNode, or @c nullptr if allocation failed.
 */
Token* NULLABLE
tokenlist_append(TokenList* list, TokenType type, const char* word) GCC_NONNULL(1, 2);

Token* NULLABLE
tokenlist_append_tok(TokenList* list, Token* token);

ASSUME_NONNULL_END
