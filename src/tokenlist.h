//
// Created by nkinder on 9/3/26.
//

#pragma once
#include "common.h"
#include "nullability.h"


/**
 * @brief Enumeration of token types.
 */
typedef enum TokenType {
    TOK_WORD,            /**< Normal text */
    TOK_ASSIGNMENT_WORD, /**< NAME=value  */
    TOK_PIPE,            /**< | */
    TOK_AND,             /**< && */
    TOK_OR,              /**< || */
    TOK_SEMI,            /**< ;  */
    TOK_AMP,             /**< &  */
    TOK_LPAREN,          /**< (  */
    TOK_RPAREN,          /**< )  */
    TOK_LBRACE,          /**< {  */
    TOK_RBRACE,          /**< }  */
    TOK_REDIR_IN,        /**< < */
    TOK_REDIR_OUT,       /**< > */
    TOK_REDIR_APPEND,    /**< >> */
    TOK_REDIR_HEREDOC,   /**< << ... EOF pattern  */
    TOK_NEWLINE,         /**< \n */
    TOK_RESERVED_WORD,   /**< if/then/else/fi/while/do/done/for/case/esac */
    TOK_CMDSUB_START,    /**< $( or ` */
    TOK_CMDSUB_END,      /**< ) or ` */
    TOK_EOF = -1,        /**< End of file */
    TOK_ERR = -2         /**< Indication of scanning error. */
} TokenType;


/**
 * @brief A single node in a @link TokenList singly linked list @endlink of tokenized words.
 */
typedef struct Token {
    TokenType              type;  /**< The type of token this is. */
    const char*            value; /**< Heap-allocated token text owned by this node. */
    int                    fd;    /**< Which file descriptor the token refers to. Only meaningful for redirects.*/
    struct Token* NULLABLE next;  /**< Next node in the list, or @c nullptr if last. */
} Token;

/**
 * @brief A singly linked list of Tokens, with an explicit element count.
 */
typedef struct TokenList {
    size_t          size; /**< Number of nodes reachable from #head. */
    Token* NULLABLE head; /**< First node in the list, or @c nullptr if empty. */
} TokenList;

ASSUME_NONNULL_BEGIN

/**
 * @brief Counts the number of tokens in a token list manually.
 *
 * The purpose of this function is mainly to deal with situations where either a TokenList's
 * size needs to be recounted (if it has a new list attached to it, for example).
 *
 * @param tokens A Non null TokenList to count.
 * @return The size of the TokenList.
 */
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
 * @brief Allocate a Token owning a copy of the given string.
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
 * @brief Free a single Token and its owned value string.
 *
 * @param node Token to free. Does not touch @c node->next; use
 *             cleanup_wordlist() to free an entire chain.
 */
void
token_delete(Token* node) GCC_NONNULL(1);

/**
 * @brief Allocate an empty TokenList.
 *
 * @return A newly allocated WordList with @c size 0 and @c head
 *         @c nullptr, or @c nullptr on allocation failure.
 */
TokenList* NULLABLE
tokenlist_new_empty();

/**
 * @brief Allocate a TokenList containing a single Token.
 *
 * @param initial_word String to duplicate as the list's first element.
 *
 * @return A newly allocated TokenList of size 1, or @c nullptr if any
 *         allocation failed.
 */
TokenList* NULLABLE
tokenlist_new(TokenType type, const char* initial_word) GCC_NONNULL(1);

TokenList*
tokenlist_copyof(TokenList* list);

/**
 * @brief Free a TokenList and every node it contains.
 *
 * @param list List to free, including all of its Token elements and
 *             their owned value strings.
 */
void
tokenlist_delete(TokenList* list) GCC_NONNULL(1);

/**
 * @brief Append a new Token to the end of a TokenList.
 *
 * @param list List to append to. If empty, the new Token becomes the head.
 * @param word String to duplicate into the newly appended Token.
 *
 * @return The newly appended Token, or @c nullptr if allocation failed.
 */
Token* NULLABLE
tokenlist_append(TokenList* list, TokenType type, const char* word) GCC_NONNULL(1, 2);

/**
 * @brief Appends an already constructed token to the list, rather than appending a word.
 *
 * Since adding a nullptr token to the end of a linked list is technically a null operation,
 * it is valid here.
 *
 * @param list A non null TokenList to append to.
 * @param token A nullable Token to append to the list.
 * @return The token which was inserted.
 */
Token* NULLABLE
tokenlist_append_tok(TokenList* list, Token* NULLABLE token) GCC_NONNULL(1);

ASSUME_NONNULL_END
