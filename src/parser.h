//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "nullability.h"
#include "tokenlist.h"
#include "parsetypes.h"
#include <stdlib.h>

#define MAX_REDIRS 10

List* NULLABLE
parse_list(TokenStream* tokens) GCC_NONNULL(1);


AndOr* NULLABLE
  parse_and_or(TokenStream* tokens)
GCC_NONNULL(1);

Pipeline* NULLABLE
parse_pipeline(TokenStream* tokens) GCC_NONNULL(1);

/**
 * @brief Parse a redirection operator and its target from tokenized words.
 *
 * Reads the redirection token from @c words->head->value (e.g. `>`,
 * `2>>`), determining the file descriptor (defaulting to 1 if none is
 * given) and mode (append vs. truncate), and takes the redirection target
 * path from @c words->head->next->value.
 *
 * @param dest  Redirect struct to populate.
 * @param words Word list whose head is the redirection operator token and
 *              whose second node is the target path.
 */
void
parse_redir(Redirect* dest, TokenStream* words) GCC_NONNULL(1, 2);


/**
 * @brief Parse a tokenized word list into a Command.
 *
 * Makes a working copy of @p words, then walks it separating redirection
 * tokens (as identified by is_redir()) from plain argument words. Each
 * redirection is parsed via parse_redir() and collected, while the
 * remaining words become the command's argv, via prepare_args() and
 * init_command().
 *
 * @param words Tokenized input words (e.g. as produced by
 *              tokenize_input()).
 *
 * @return A newly allocated Command ready for execution, or @c nullptr on
 *         allocation failure.
 */
Command* NULLABLE
parse_command(TokenStream* stream) GCC_NONNULL(1);
/**
 *  @brief Destructively splits a list in two at a pipe.
 *
 *  @attention for a command @code
 *  Command args | Command2 args | Command3 args
 *  @endcode
 *  Calling this function will result in a dest which is @code
 *  Command2 args | Command3 args
 *  @endcode
 *  and a src which is @code
 *  Command args
 *  @endcode
 *  That is, everything after the first pipe is moved to the new list.
 *
 * @param dest The destination for the new split.
 * @param src The word list to split. This is a destructive operation
 * @return true if any splitting happened.
 */
bool
split_on_pipes(TokenList* dest, TokenList* src) GCC_NONNULL(1, 2);


size_t
count_pipes(TokenList* tokens) GCC_NONNULL(1);

/**
 * @brief Extract the next whitespace/quote-aware token from a buffer.
 *
 * Scans @p buf starting at its beginning, honoring single/double quoting
 * and backslash escaping according to @p flag, and writes the decoded
 * token into @p dest. Unquoted runs of spaces separate tokens and are
 * consumed but not included in the output. @p flag is updated in place to
 * reflect the quoting state at the end of the scan (e.g. left as
 * @c SINGLE_QUOTED if a closing quote was never found).
 *
 * @param dest Buffer to receive the decoded, null-terminated token.
 * @param buf  Input buffer to scan; not modified.
 * @param flag In/out quoting state, carried across successive calls when
 *             tokenizing a larger input.
 *
 * @return Number of bytes consumed from @p buf, or 0 if no token was
 *         produced (e.g. end of input).
 */
size_t
next_token(char* dest, char* buf, QuoteFlagE* flag) GCC_NONNULL(1, 2, 3);

TokenType token(char* buf, const char* input, QuoteFlagE* flag);
/**
 * @brief Tokenize a full line of input into a WordList.
 *
 * Repeatedly calls next_token() over @p buf to split it into words,
 * respecting quoting, then runs the resulting list through expansion
 * (variable/command substitution) before returning it.
 *
 * @param buf Null-terminated input line to tokenize. Not modified.
 *
 * @return A newly allocated WordList of expanded tokens, or @c nullptr if
 *         allocation failed or the input contained an unterminated quote
 *         (in which case an error is printed to stderr).
 */
TokenList* NULLABLE
tokenize_input(const char* buf) GCC_NONNULL(1);

/**
 * @brief Split a colon-separated PATH-style string into a WordList.
 *
 * @param path Colon-delimited string (as returned by
 *             `getenv("PATH")`, for example).
 *
 * @return A newly allocated WordList containing each ':'-delimited
 *         segment of @p path as a separate word.
 */
TokenList* NULLABLE
tokenize_path(const char* path) GCC_NONNULL(1);

/**
 * @brief Copy word values from a WordList into a plain argv-style array.
 *
 * @param dest  Array of pre-allocated buffers, one per word in @p words,
 *              each large enough to hold that word's text plus a null
 *              terminator. Must have at least @c words->size + 1 entries
 *              so the terminating @c nullptr can be written.
 * @param words List of words to copy, in list order.
 */
void
prepare_args(char** NULLABLE dest, TokenList* words) GCC_NONNULL(1, 2);


ASSUME_NONNULL_END
