//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "nullability.h"
#include "parsetypes.h"
#include "tokenlist.h"
#include <stdlib.h>

#define MAX_REDIRS 10

List* NULLABLE
lex_and_parse(const char* input);

List* NULLABLE
parse_list(TokenStream* tokens) GCC_NONNULL(1);

AndOr* NULLABLE
parse_and_or(TokenStream* tokens) GCC_NONNULL(1);

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

void
assignment_split(char* dest[2], const char* assignment);

ASSUME_NONNULL_END
