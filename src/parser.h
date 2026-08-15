//
// Created by nkinder on 8/13/26.
//

#pragma once
#include <stdlib.h>
#include <linux/limits.h>

#define MAX_REDIRS 1;

/**
 * @brief Tracks the current quoting state while tokenizing input.
 *
 * @c DOUBLE_QUOTED is defined as @c 0x3 so that it overlaps the bit
 * pattern of @c SINGLE_QUOTED (0x1); code that only needs to know "some
 * kind of quoting is active" can test with a bitwise AND against
 * @c SINGLE_QUOTED.
 */
typedef enum QuoteFlagE {
    UNQUOTED,           /**< No quoting is currently active. */
    SINGLE_QUOTED,      /**< Inside a single-quoted (`'...'`) span. */
    DOUBLE_QUOTED = 0x3 /**< Inside a double-quoted (`"..."`) span. */
} QuoteFlagE;

/**
 * @brief Mode for a single I/O redirection.
 *
 * Values are chosen to double as flags that can be OR'd directly into the
 * @c flags argument of @c open() alongside @c O_WRONLY / @c O_CREAT.
 */
typedef enum RedirMode {
    REDIR_IN,             /**< Input redirection (`<`). */
    REDIR_OUT    = 01000, /**< Output redirection, truncating (`>`). */
    REDIR_APPEND = 02000  /**< Output redirection, appending (`>>`). */
} RedirMode;

#ifdef __clang__
#pragma clang assume_nonnull begin

/**
 * @brief A single parsed I/O redirection.
 */
typedef struct Redirect {
    int       fd;     /**< File descriptor being redirected (e.g. 1 for stdout). */
    RedirMode mode;   /**< Redirection mode (in / out / append). */
    char*     target; /**< Heap-allocated path of the redirection target file. */
} Redirect;

/**
 * @brief A fully parsed shell command, ready for execution.
 */
typedef struct Command {
    char* _Nullable * argv;     /**< Null-terminated, heap-allocated argument vector. */
    size_t            nredirs;  /**< Number of entries in #redirs. */
    Redirect          redirs[]; /**< Flexible array of redirections attached to this command. */
} Command;

/**
 * @brief A single node in a singly linked list of tokenized words.
 */
typedef struct WordNode {
    const char*                value; /**< Heap-allocated token text owned by this node. */
    struct WordNode* _Nullable next;  /**< Next node in the list, or @c nullptr if last. */
} WordNode;

/**
 * @brief A singly linked list of tokens, with an explicit element count.
 */
typedef struct WordList {
    size_t              size; /**< Number of nodes reachable from #head. */
    WordNode* _Nullable head; /**< First node in the list, or @c nullptr if empty. */
} WordList;

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
WordList* _Nullable
new_from_nodes(WordNode* head);

/**
 * @brief Allocate a Command from a raw argv array and redirection list.
 *
 * Duplicates each string in @p argv (up to the null terminator) into a
 * freshly allocated argv array, and copies @p nredirs entries from
 * @p redirs into the Command's trailing flexible array.
 *
 * @param argv    Null-terminated array of argument strings to copy.
 * @param nredirs Number of redirections in @p redirs.
 * @param redirs  Array of @p nredirs redirections to copy into the command.
 *
 * @return A newly allocated Command, or @c nullptr if allocation or string
 *         duplication failed (in which case any partial allocations are
 *         freed before returning).
 */
Command* _Nullable
init_command(char* _Nullable * argv, size_t nredirs, Redirect* redirs);

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
Command* _Nullable
build_command(WordList* words);

/**
 * @brief Free a Command and its owned argv strings.
 *
 * @param command Command previously returned by init_command() or
 *                build_command().
 */
void
cleanup_command(Command* command);

/**
 * @brief Allocate a standalone Redirect.
 *
 * @param fd     File descriptor to redirect.
 * @param mode   Redirect mode.
 * @param target Redirection target path; this string is duplicated, so the
 *               caller retains ownership of @p target itself.
 *
 * @return A newly allocated Redirect object, or @c nullptr if allocation
 *         or string duplication failed.
 */
Redirect* _Nullable
init_redirect(int fd, RedirMode mode, const char* target);

/**
 * @brief Free a Redirect previously returned by init_redirect().
 *
 * @param redir Redirect to free, including its owned @c target string.
 */
void
cleanup_redirect(Redirect* redir);

/**
 * @brief Allocate a WordNode owning a copy of the given string.
 *
 * @param initial_word String to duplicate into the new node.
 *
 * @return A newly allocated WordNode with @c next set to @c nullptr, or
 *         @c nullptr if allocation or duplication failed.
 */
WordNode* _Nullable
init_wordnode(const char* initial_word);

/**
 * @brief Free a single WordNode and its owned value string.
 *
 * @param node Node to free. Does not touch @c node->next; use
 *             cleanup_wordlist() to free an entire chain.
 */
void
cleanup_wordnode(WordNode* node);

/**
 * @brief Allocate an empty WordList.
 *
 * @return A newly allocated WordList with @c size 0 and @c head
 *         @c nullptr, or @c nullptr on allocation failure.
 */
WordList* _Nullable
empty_wordlist();

/**
 * @brief Allocate a WordList containing a single word.
 *
 * @param initial_word String to duplicate as the list's first element.
 *
 * @return A newly allocated WordList of size 1, or @c nullptr if any
 *         allocation failed.
 */
WordList* _Nullable
init_wordlist(const char* initial_word);

/**
 * @brief Free a WordList and every node it contains.
 *
 * @param list List to free, including all of its WordNode elements and
 *             their owned value strings.
 */
void
cleanup_wordlist(WordList* list);

/**
 * @brief Append a new word to the end of a WordList.
 *
 * @param list List to append to. If empty, the new node becomes the head.
 * @param word String to duplicate into the newly appended node.
 *
 * @return The newly appended WordNode, or @c nullptr if allocation failed.
 */
WordNode* _Nullable
append_wordlist(WordList* list, const char* word);

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
next_token(char* dest, char* buf, QuoteFlagE* flag);

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
WordList* _Nullable
tokenize_input(const char* buf);

/**
 * @brief Split a colon-separated PATH-style string into a WordList.
 *
 * @param path Colon-delimited string (as returned by
 *             `getenv("PATH")`, for example).
 *
 * @return A newly allocated WordList containing each ':'-delimited
 *         segment of @p path as a separate word.
 */
WordList* _Nullable
tokenize_path(const char* path);

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
prepare_args(char** _Nullable dest, WordList* words);

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
parse_redir(Redirect* dest, WordList* words);

#pragma clang assume_nonnull end

#endif //ifdef __clang__