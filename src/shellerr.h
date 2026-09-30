//
// Created by nkinder on 9/26/26.
//

#pragma once
#include <errno.h>

#define SHERR_BEGIN 256
/**
 * Represents various exceptional or illegal states and operations for the shell.
 * SH_ERRNO indicates that the error is mirrored by the errno.h module, and
 * sys_errno should be used to get a string. The range of shell specific errors
 * begins at 256, which means you can use 0x100 >> 8 to test if the value is in
 * the shell specific range.
 *
 */
typedef enum ShellErrorCode {
    SH_NOERR = 0, //< No error
    SH_ERRNO,     //< Sys errno value is set and should be used to obtain an error string
    // Shell reserved errnos
    SH_COMMAND_NOT_FOUND = SHERR_BEGIN, //< Command not found.
    SH_UNEXPECTED_TOKEN,                //< Unexpected token
    SH_TRAILING_OPERATOR,               //< Trailing operator with nothing after
    SH_UNTERMINATED_DOUBLE_QUOTE,       //< Unterminated " quotation
    SH_UNTERMINATED_SINGLE_QUOTE,       //< Unterminated ' quotation
    SH_UNTERMINATED_PARENTHESES,        //< Unterminated Parentheses $(...)
    SH_UNTERMINATED_BRACES,             //< Unterminated braces in ${...}
    SH_MISSING_REDIRECTION_TARGET,      //< Redirection operator with no target
    SH_INVALID_ARGUMENT,                //< Invalid argument to a builtin
    SH_INVALID_IDENTIFIER,              //< Invalid identifier in assignment
    SH_INVALID_FD,                      //< Invalid fd
    SH_TRAILING_ESCAPE,                 //< Trailing '\'
    SH_INVALID_COMMAND_SUBSTITUTION,    //< Invalid command substitution
    SH_COMMAND_EXPECTED,                //< Expected a command
    SH_INVALID_EXEC_FORMAT,             //< Invalid exec format
    SH_NO_JOB,                          //< No job
    SH_NO_CURRENT_JOB,                  //< No current job
    SH_JOB_RUNNING,                     //< Job already running
    SH_COUNT                            //< Ending point for shell reserved.
} ShellErrorCode;

typedef struct ShellError {
    ShellErrorCode code;
    int            sys_errno;
    char*          context;
} ShellError;

/**
 * @brief Reset an error to the "no error" state, freeing any owned context.
 *
 * Safe to call on an already-clear ShellError.
 *
 * @param err Error to clear. Caller retains ownership of the struct itself
 *            (this does not free @p err, only its contents).
 */
void
shell_error_clear(ShellError* error);

/**
 * @brief Set a shell-native error, with an optional formatted context
 * string.
 *
 * Clears any previous content of @p err first.
 *
 * @param err        Error to populate.
 * @param code       The shell-native error code. Must not be SHERR_ERRNO;
 *                    use shell_error_set_errno() for that.
 * @param context_fmt Optional printf-style format for the context string,
 *                    or nullptr for no context. Interpreted with the
 *                    trailing varargs.
 */
void
shell_error_set(ShellError* err, ShellErrorCode code, const char* context_fmt, ...);

/**
 * @brief Set an error wrapping a specific errno value.
 *
 * Use this (rather than shell_error_set_from_errno()) when the failing
 * call's errno was already captured into a local, to avoid clobbering it
 * with an intervening call.
 *
 * @param err        Error to populate.
 * @param sys_errno  The errno value to wrap (e.g. saved right after the
 *                    failing syscall).
 * @param context_fmt Optional printf-style context, or nullptr.
 */
void
shell_error_set_errno(ShellError* err, int sys_errno, const char* context_fmt, ...);

/**
 * @brief Convenience: wraps the CURRENT value of the global @c errno.
 *
 * Equivalent to `shell_error_set_errno(err, errno, ...)`. Call this
 * immediately after the failing syscall -- anything in between
 * (including most libc calls) may clobber errno first.
 */
void
shell_error_set_from_current_errno(ShellError* err, const char* context_fmt, ...);

/**
 * @brief Check whether an error is currently set.
 *
 * @return True if @p err->code != SHERR_NONE.
 */
bool
shell_error_is_set(const ShellError* err);

/**
 * @brief Render an error into a human-readable message.
 *
 * For SHERR_ERRNO, delegates to strerror() on sys_errno. For shell-native
 * codes, uses a static message table. If @c context is set, it's woven
 * into the message (exact phrasing is code-specific -- e.g. "ls: command
 * not found" vs "unexpected token '|'").
 *
 * @param err Error to render. Must have code != SHERR_NONE.
 *
 * @return A newly heap-allocated message string; caller must free() it.
 */
char*
shell_error_message(const ShellError* err);


/**
 * @brief Print an error to stderr in the shell's standard format
 * (`shell: <message>`), and clear it afterward.
 *
 * Equivalent to `fputs(shell_error_message(err), stderr)` plus cleanup;
 * provided as the single place that owns the on-screen error format, so
 * every call site doesn't reformat independently.
 *
 * @param err Error to report and clear.
 */
void
shell_error_report(ShellError* err);

/**
 * @brief Map an error to the shell's conventional process exit code.
 *
 * Mirrors bash's exit-code conventions: SHERR_COMMAND_NOT_FOUND -> 127,
 * SHERR_PERMISSION_DENIED -> 126, parse/lex errors -> 2, everything else
 * (including SHERR_ERRNO) -> 1. Intended for whatever code path turns a
 * ShellError into the shell's own $?.
 *
 * @param err Error to map. SHERR_NONE maps to 0.
 */
int
shell_error_exit_code(const ShellError* err);

/**
 * The global error status. Used similarly to errno.
 */
extern ShellError shell_errno;

/** Convenience wrapper: shell_error_set(&shell_errno, ...). */
#define shell_seterr(code, ...) shell_error_set(&shell_errno, (code), __VA_ARGS__)


/** Convenience wrapper: shell_error_report(&shell_errno). */
#define shell_reporterr() shell_error_report(&shell_errno)
