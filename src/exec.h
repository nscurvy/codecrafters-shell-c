//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "builtins.h"
struct Command;

/**
 * @brief Locate an executable on the system PATH.
 *
 * Iterates over each directory listed in the @c PATH environment variable,
 * appending @p command to it, and checks whether the resulting path exists
 * and is executable by the user, group, or others. The first matching path
 * is copied into @p dest.
 *
 * @param dest    Buffer to receive the resolved absolute path. Must be large
 *                enough to hold a full path (e.g. @c PATH_MAX bytes). On
 *                failure to resolve, this buffer is left empty
 *                (null-terminated at index 0).
 * @param command Name of the command to search for (not a path).
 *
 * @return Pointer to @p dest if an executable match was found, or @c nullptr
 *         if the command could not be located on @c PATH (or @c PATH is
 *         unset).
 */

char* find_command(char* dest, const char* command);

/**
 * @brief Execute an external (non-builtin) command in a child process.
 *
 * Forks the current process. In the child, applies the first redirect in
 * @p command (if any) via @c dup2, then replaces the process image using
 * @c execvp with @p command's argv. The parent waits for the child to exit.
 *
 * @param command Parsed command to execute, including argv and any
 *                redirections.
 *
 * @return 0 if the fork and wait succeeded (regardless of the child's exit
 *         status), or 1 if @c fork() failed.
 *
 * @note If @c execvp fails in the child, the child process exits/returns
 *       with status 1 rather than returning control to the caller.
 */
int execc(const struct Command* command);

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
BuiltinCmd* find_builtin(const char* name);

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
int repl();
struct WordList;

/**
 * TODO: Documentation
 * @brief
 * @param dest
 * @param words
 */
void prepare_args(char** dest, struct WordList* words);

/**
 * TODO: this
 * @brief
 * @param command
 * @return
 */
int execute_command(struct Command* command);
