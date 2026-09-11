//
// Created by nkinder on 9/10/26.
//

#pragma once
#include "tokenlist.h"

/* *****************************************************************************
 * TokenStream
 * ****************************************************************************/

/**
 * A non-owning stream adapter for tokens.
 */
typedef struct TokenStream {
    size_t len;      /**< The length of the stream. */
    size_t pos;      /**< The current position within the stream. */
    Token* tokens[]; /**< The underlying array of tokens. */
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
 * @brief Return the next token without consuming the stream.
 *
 * @param stream Stream
 * @return The next token in the stream.
 */
Token*
ts_peek(TokenStream* stream) GCC_NONNULL(1);

/**
 * @brief Peek n positions ahead in the stream.
 *
 * @param stream Stream
 * @param n The number of positions to peek ahead by
 * @return The token n positions away from the current position, or the final token if that jump exceeds the stream
 * size.
 */
Token*
ts_peek_ahead(TokenStream* stream, size_t n) GCC_NONNULL(1);

/**
 * @brief Consumes a token from the stream. If the stream is exhausted, this returns
 * the final token of the stream. This should always return a valid token so long as the
 * list that constructed it is valid.
 *
 * @param stream Stream
 * @return The next token, or the final token
 */
Token*
ts_read(TokenStream* stream) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type, do <i>not</i> consume the stream.
 *
 * @param stream Stream
 * @param type Desired type
 * @return True if the type matches, false otherwise
 */
bool
ts_check(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type and consumes it if so.
 *
 * If not, this function returns false.
 *
 * @param stream Stream
 * @param type The desired type
 * @return True if a match, false otherwise
 */
bool
ts_match(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the next token matches the given type and consumes it if so.
 *
 * If not, this function sets errno and returns nullptr so that a parse error can be reported.
 *
 * @param stream TokenStream
 * @param type The expected type
 * @return The next token, or nullptr if there was an error.
 */
Token* NULLABLE
ts_expect(TokenStream* stream, TokenType type) GCC_NONNULL(1);

/**
 * @brief Check if the stream has been exhausted.
 *
 * @param stream TokenStream
 * @return True if at the end.
 */
bool
ts_at_end(TokenStream* stream) GCC_NONNULL(1);

/**
 * @brief Query the TokenStream about how many more Tokens remain.
 *
 * @param stream The stream
 * @return The number of remaining tokens.
 */
size_t
ts_remaining(TokenStream* stream);

/**
 * @brief Delete the TokenStream. This function does not delete the underlying Token objects,
 * which are the responsibility of the TokenList which constructed the stream.
 *
 * @param tokens The stream being deleted
 */
void
ts_delete(TokenStream* tokens) GCC_NONNULL(1);

/* *****************************************************************************
 * Redirect
 * ****************************************************************************/

/**
 * @brief Mode for a single I/O redirection.
 *
 * Values are chosen to double as flags that can be OR'd directly into the
 * @c flags argument of @c open() alongside @c O_WRONLY / @c O_CREAT.
 */
typedef enum RedirMode {
    REDIR_IN      = 0b00, /**< Input redirection (`<`). */
    REDIR_OUT     = 0b10, /**< Output redirection, truncating (`>`). */
    REDIR_APPEND  = 0b01, /**< Output redirection, appending (`>>`). */
    REDIR_HEREDOC = 0b11, /**< Input redirection, heredoc (<< EOF). */
    REDIR_ERR     = 0b100 /**< Error value.*/
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
 * @brief Free a Redirect previously returned by redir_new().
 *
 * @param redir Redirect to free, including its owned @c target string.
 */
void
redir_delete(Redirect* redir) GCC_NONNULL(1);

/* *****************************************************************************
 * Assignment
 * ****************************************************************************/

/**
 * An Assignment Token. For example, PATH=/bin would be an assignment. These get attached
 * to commands so that they can be run before execution.
 */
typedef struct Assignment {
    const char*        value; /**< The literal value of the Token */
    struct Assignment* next;  /**< The next Assignment in the list. */
} Assignment;

/**
 * Create a new assignment object, copying the value argument into its own buffer.
 *
 * @param value The token value, e.g. PATH=abc
 * @return A new Assignment object or nullptr if it fails.
 */
Assignment*
ass_new(const char* value);

/**
 * Append a node to the end of the given list.
 *
 * @param head The head of the list
 * @param node The node to append
 */
void
ass_append(Assignment* head, Assignment* node);

/**
 * Append the given string to the end of the head. This function will return the node
 * constructed from the given value, unless there is an allocation failure, in which case it
 * will return nullptr
 *
 * @param head The head of the list.
 * @param value The string to append
 * @return The appended node, or nullptr
 */
Assignment*
ass_append_str(Assignment* head, const char* value);

/**
 * Delete this assignment node. This does not delete anything up the chain.
 *
 * @param assignment The node to delete
 */
void
ass_delete(Assignment* assignment);

/**
 * Delete this assignment list, meaning the head node and every node that follows it.
 *
 * @param head The head of the list
 */
void
ass_delete_all(Assignment* head);

/* *****************************************************************************
 * Command
 * ****************************************************************************/

/**
 * @brief A fully parsed shell command, ready for execution.
 */
typedef struct Command {
    char* NULLABLE* argv; /**< Null-terminated, heap-allocated argument vector. */
    Assignment*     assignment_list;
    size_t          nredirs;  /**< Number of entries in #redirs. */
    Redirect        redirs[]; /**< Flexible array of redirections attached to this command. */
} Command;

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
Command* NULLABLE
command_new(const char* NULLABLE* argv, Assignment* NULLABLE assignments, size_t nredirs, Redirect* redirs)
        GCC_NONNULL(4);


/**
 * @brief Free a Command and its owned argv strings.
 *
 * @param command Command previously returned by init_command() or
 *                build_command().
 */
void
command_delete(Command* command) GCC_NONNULL(1);


/* *****************************************************************************
 * Pipeline
 * *****************************************************************************/

/**
 * An element in a Pipeline
 */
typedef struct PipelineElement {
    Command*                command; /**< The Command being represented. */
    struct PipelineElement* next;    /**< The next in the list. */
} PipelineElement;

/**
 * The representation of a Command Pipeline. They are collections of commands delimited
 * by the '|' operator.
 *
 */
typedef struct Pipeline {
    size_t           size; /**< The size of the Pipeline list. */
    PipelineElement* head; /**< The list's head. */
} Pipeline;

/**
 * Construct a new, empty pipeline.
 *
 * @return A new, empty Pipeline. Or null if it fails.
 */
Pipeline* NULLABLE
pipeline_new_empty();

/**
 * Delete an allocated Pipeline.
 *
 * @param pipeline The Pipeline to delete.
 */
void
pipeline_delete(Pipeline* pipeline) GCC_NONNULL(1);

/**
 * Construct a new PipelineElement.
 *
 * @param command The Command this element represents.
 * @return A new PipelineElement, or null if it fails.
 */
PipelineElement*
ple_new(Command* command);

/**
 * Delete a PipelineElement
 *
 * @param element The element to delete.
 */
void
ple_delete(PipelineElement* element);


/* *****************************************************************************
 * And/Or
 * ****************************************************************************/

/**
 * Enumeration describing delimitations of Command Pipelines.
 */
typedef enum AndOrOpE {
    AND_NONE, /**< This is always the first operation in a chain. "No operator has preceded this command" */
    AND_AND,  /**< Represents an && operator*/
    AND_OR    /**< Represents an || operator */
} AndOrOp;

/**
 * Representation of an element in an AndOr chain.
 *
 */
typedef struct AndOrElement {
    Pipeline*            pipeline; /**< The pipeline which comes after this operator. */
    AndOrOp              op;       /**< The operator which preceded this pipeline. */
    struct AndOrElement* next;
} AndOrElement;

/**
 * @brief Model of a series of Command Pipelines separated by the control operators '||', '&&', and ';'
 *
 */
typedef struct AndOr {
    size_t        count; /**< The number of pipeline operations. */
    AndOrElement* head;
} AndOr;

AndOr* NULLABLE
ao_new_empty();

void
ao_delete(AndOr* ao) GCC_NONNULL(1);

AndOrElement* NULLABLE
aoe_new(Pipeline* pipeline, AndOrOp op) GCC_NONNULL(1);

void
aoe_delete(AndOrElement* aoe) GCC_NONNULL(1);


/* *****************************************************************************
 * List
 * ****************************************************************************/

/**
 * Enumeration representing the delimiting operation (&,;) at the end of the List.
 */
typedef enum ListSepE { SEP_SEMI, SEP_AMP, SEP_NONE } ListSep;

typedef struct ListElement {
    AndOr*              and_or;
    ListSep             sep;
    struct ListElement* next;
} ListElement;

typedef struct List {
    size_t       count;
    ListElement* head;
} List;

ListElement*
le_new(AndOr* ao, ListSep sep) GCC_NONNULL(1);

void
le_delete(ListElement* le) GCC_NONNULL(1);

List* NULLABLE
list_new_empty();

void
list_delete(List* list) GCC_NONNULL(1);
