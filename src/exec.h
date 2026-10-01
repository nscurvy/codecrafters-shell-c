//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "builtins.h"
#include "nullability.h"
ASSUME_NONNULL_BEGIN
struct List;
struct AndOr;
struct Command;
struct Pipeline;


/**
 * @brief Look up a shell builtin by name.
 *
 * Performs a binary search over the global @c builtins table (which must be
 * sorted for this to work correctly) to find a builtin command matching
 * @p name.
 *
 * @param name Name of the builtin to search for.
 *
 * @return Pointer to the matching @c BuiltinCmd entry, or @c nullptr if no
 *         builtin with that name exists.
 */
BuiltinCmd* NULLABLE
find_builtin(const char* name) GCC_NONNULL(1);

/**
 * @brief Run the shell's interactive read-eval-print loop.
 *
 * Repeatedly prompts for input, tokenizes and parses it into a @c Command,
 * and either invokes a matching builtin (applying any redirections around
 * the call) or resolves and executes an external command via find_command()
 * and execc(). Loops indefinitely.
 *
 * @return This function currently runs an infinite loop and does not
 *         return under normal operation.
 */
int
repl();
struct TokenList;

/**
 * TODO: Documentation
 * @brief
 * @param dest
 * @param words
 */
void
prepare_args(char** dest, struct TokenList* words) GCC_NONNULL(1, 2);

/**
 * TODO: this
 * @brief
 * @param command
 * @return
 */
int
execute_command(struct Command* command, bool foreground) GCC_NONNULL(1);

int
execute_list(struct List* list);

int
execute_andor(struct AndOr* andor, bool foreground);

int
execute_andor_bg(struct AndOr* andor);

int
execute_pipeline(struct Pipeline* pipeline, bool foreground);

ASSUME_NONNULL_END
