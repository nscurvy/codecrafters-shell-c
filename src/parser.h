//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "nullability.h"
#include "tokenlist.h"
#include <stdlib.h>

#define MAX_REDIRS 10

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

ASSUME_NONNULL_BEGIN
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
    char* NULLABLE* argv; /**< Null-terminated, heap-allocated argument vector. */
    bool            bgjob;
    size_t          nredirs;  /**< Number of entries in #redirs. */
    Redirect        redirs[]; /**< Flexible array of redirections attached to this command. */
} Command;

/**
 *  TODO: Document
 * @brief
 */
typedef struct Pipeline {
    size_t   ncmds;
    Command* cmds[];
} Pipeline;


/**
 * Enumeration describing delimitations of Command Pipelines.
 */
typedef enum AndOrOpE {
  AND_NONE, /**< This is always the first operation in a chain. "No operator has preceded this command" */
  AND_AND, /**< Represents an && operator*/
  AND_OR /**< Represents an || operator */
} AndOrOp;

/**
 * Representation of an element in an AndOr chain.
 *
 */
typedef struct AndOrElement {
  Pipeline* pipeline; /**< The pipeline which comes after this operator. */
  AndOrOp op; /**< The operator which preceded this pipeline. */
} AndOrElement;

/**
 * @brief Model of a series of Command Pipelines separated by the control operators '||', '&&', and ';'
 *
 */
typedef struct AndOr {
  size_t count; /**< The number of pipeline operations. */
  AndOrElement elements[];
} AndOr;

typedef enum ListSepE {
  SEP_SEMI,
  SEP_AMP,
  SEP_NONE
} ListSep;
typedef struct ListElement {
  AndOr* and_or;
  ListSep sep;
} ListElement;

typedef struct List {
  size_t count;
  ListElement elements[];
} List;

typedef struct TokenStream {
  size_t len;
  size_t pos;
  Token* tokens[];
} TokenStream;

/**
 * @brief Construct a new TokenStream object. This does not consider itself an owner of
 * the underlying tokens and it does not copy any.
 *
 * @param tokens The TokenList to adapt into a stream
 * @return A new TokenStream, or nullptr in the event of allocation error.
 */
TokenStream* NULLABLE
ts_new(TokenList* tokens) GCC_NONNULL(1);

/**
 * @brief Delete the TokenStream. This function does not delete the underlying Token objects,
 * which are the responsibility of the TokenList which constructed the stream.
 * @param tokens The stream being deleted
 */
void
ts_delete(TokenStream* tokens) GCC_NONNULL(1);

/**
 * @brief Return the next token without consuming the stream.
 *
 * @param stream Stream
 * @return The next token in the stream.
 */
Token* ts_peek(TokenStream* stream) GCC_NONNULL(1);

/**
 * @brief Peek n positions ahead in the stream.
 *
 * @param stream Stream
 * @param n The number of positions to peek ahead by
 * @return The token n positions away from the current position, or the final token if that jump exceeds the stream size.
 */
Token* ts_peek_ahead(TokenStream* stream, size_t n) GCC_NONNULL(1);

/**
 * @brief Consumes a token from the stream. If the stream is exhausted, this returns
 * the final token of the stream. This should always return a valid token so long as the
 * list that constructed it is valid.
 *
 * @param stream Stream
 * @return The next token, or the final token
 */
Token* ts_read(TokenStream* stream) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type, do <i>consume</i> the stream.
 *
 * @param stream Stream
 * @param type Desired type
 * @return True if the type matches, false otherwise
 */
bool ts_check(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type and consumes it if so.
 *
 * If not, this function returns false.
 *
 * @param stream Stream
 * @param type The desired type
 * @return True if a match, false otherwise
 */
bool ts_match(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type and consumes it if so.
 *
 * If not, this function sets errno and returns nullptr so that a parse error can be reported.
 *
 * @param stream TokenStream
 * @param type The expected type
 * @return The next token, or nullptr if there was an error.
 */
Token* NULLABLE ts_expect(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the stream has been exhausted.
 *
 * @param stream TokenStream
 * @return True if at the end.
 */
bool ts_at_end(TokenStream* stream) GCC_NONNULL(1);

List* NULLABLE
list_new(size_t count, ListElement elements[]);

void
list_delete(List* list) GCC_NONNULL(1);

List* NULLABLE
parse_list(TokenList* tokens) GCC_NONNULL(1);


AndOrElement* NULLABLE
aoe_new(Pipeline* pipeline, AndOrOp op)
GCC_NONNULL(1);

void
aoe_delete(AndOrElement* aoe)
GCC_NONNULL(1);

AndOr* NULLABLE
ao_new(size_t count, AndOrElement elements[])
GCC_NONNULL(2);

void
ao_delete(AndOr* ao)
GCC_NONNULL(1);

AndOr* NULLABLE
  parse_and_or(TokenList* tokens)
GCC_NONNULL(1);

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

Pipeline* NULLABLE
parse_pipeline(TokenList* tokens) GCC_NONNULL(1);

Pipeline* NULLABLE
pipeline_new(size_t ncmds, Command** cmds) GCC_NONNULL(2);

void
pipeline_delete(Pipeline* pipeline) GCC_NONNULL(1);


/**
 * @brief Allocate a Command from a raw argv array and redirection list.
 *
 * Duplicates each string in @p argv (up to the null terminator) into a
 * freshly allocated argv array, and copies @p nredirs entries from
 * @p redirs into the Command's trailing flexible array.
 *
 * @param argv    Null-terminated array of argument strings to copy.
 * @param bgjob
 * @param nredirs Number of redirections in @p redirs.
 * @param redirs  Array of @p nredirs redirections to copy into the command.
 *
 * @return A newly allocated Command, or @c nullptr if allocation or string
 *         duplication failed (in which case any partial allocations are
 *         freed before returning).
 */
Command* NULLABLE
command_new(char* NULLABLE* argv, bool bgjob, size_t nredirs, Redirect* redirs);

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
parse_command(TokenList* words) GCC_NONNULL(1);

/**
 * @brief Free a Command and its owned argv strings.
 *
 * @param command Command previously returned by init_command() or
 *                build_command().
 */
void
command_delete(Command* command) GCC_NONNULL(1);

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
Redirect* NULLABLE
redir_new(int fd, RedirMode mode, const char* target) GCC_NONNULL(3);

/**
 * @brief Free a Redirect previously returned by init_redirect().
 *
 * @param redir Redirect to free, including its owned @c target string.
 */
void
redir_delete(Redirect* redir) GCC_NONNULL(1);


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
parse_redir(Redirect* dest, TokenList* words) GCC_NONNULL(1, 2);

ASSUME_NONNULL_END
