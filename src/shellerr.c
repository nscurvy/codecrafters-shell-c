//
// Created by nkinder on 9/26/26.
//

#include "shellerr.h"

#include "common.h"
#include <stdarg.h>

ShellError shell_errno;

typedef struct ErrorTemplate {
    const char* with_context;
    const char* no_context;
} ErrorTemplate;

#define ERRIDX(code) [(code) - SHERR_BEGIN]

#define ERROR_INDEX(code) ((size_t) (code) - SHERR_BEGIN)

static const ErrorTemplate templates[SH_COUNT - SHERR_BEGIN] = {
        ERRIDX(SH_COMMAND_NOT_FOUND) = {"%s: command not found", "command not found"},
        ERRIDX(SH_UNEXPECTED_TOKEN) = {"syntax error near unexpected token `%s`", "syntax error near unexpected token"},
        ERRIDX(SH_TRAILING_OPERATOR)            = {"syntax error: trailing operator with no operand on `%s`",
                                                   "syntax error: trailing operator"},
        ERRIDX(SH_UNTERMINATED_DOUBLE_QUOTE)    = {"syntax error: unterminated double quote near `%s`",
                                                   "syntax error: unterminated double quote"},
        ERRIDX(SH_UNTERMINATED_SINGLE_QUOTE)    = {"syntax error: unterminated single quote near `%s`",
                                                   "syntax error: unterminated single quote"},
        ERRIDX(SH_UNTERMINATED_PARENTHESES)     = {"syntax error: unterminated parentheses near `%s`",
                                                   "syntax error: unterminated parentheses"},
        ERRIDX(SH_UNTERMINATED_BRACES)          = {"syntax error: unterminated braces near `%s`",
                                                   "syntax error: unterminated braces"},
        ERRIDX(SH_MISSING_REDIRECTION_TARGET)   = {"syntax error: missing redirection target near `%s`",
                                                   "syntax error: missing redirection target"},
        ERRIDX(SH_TRAILING_ESCAPE)              = {"syntax error: trailing backslash after `%s`",
                                                   "syntax error: trailing backslash"},
        ERRIDX(SH_INVALID_COMMAND_SUBSTITUTION) = {"syntax error: unterminated command substitution near `%s`",
                                                   "syntax error: unterminated command substitution"},
        ERRIDX(SH_INVALID_ARGUMENT)             = {"%s: invalid argument", "invalid argument"},
        ERRIDX(SH_INVALID_IDENTIFIER)           = {"%s: not a valid identifier", "not a valid identifier"},

        ERRIDX(SH_INVALID_FD)          = {"%s: invalid file descriptor", "invalid file descriptor"},
        ERRIDX(SH_COMMAND_EXPECTED)    = {"syntax error: expected a command near `%s`",
                                          "syntax error: expected a command"},
        ERRIDX(SH_INVALID_EXEC_FORMAT) = {"%s: exec format error", "exec format error"},

        ERRIDX(SH_NO_JOB)         = {"%s: no such job", "no such job"},
        ERRIDX(SH_NO_CURRENT_JOB) = {"%s: no current job", "no current job"},
        ERRIDX(SH_JOB_RUNNING)    = {"job %s already in background", "job already in background"},
};

char*
ctxstr_new(const char* fmt, va_list args) {
    va_list cpy;
    va_copy(cpy, args);
    size_t buffer_size   = (size_t) vsnprintf(nullptr, 0, fmt, cpy);
    char*  context       = malloc(sizeof(char) * buffer_size + 1);
    context[buffer_size] = 0;
    va_end(cpy);
    vsnprintf(context, buffer_size + 1, fmt, args);
    return context;
}

void
shell_error_clear(ShellError* error) {
    error->code      = 0;
    error->sys_errno = 0;
    if (error->context) {
        free(error->context);
    }
    error->context = nullptr;
}

void
shell_error_set(ShellError* err, ShellErrorCode code, const char* context_fmt, ...) {
    shell_error_clear(err);
    char* context = nullptr;
    if (context_fmt) {
        va_list args = (va_list){0};
        va_start(args, context_fmt);
        context = ctxstr_new(context_fmt, args);
        va_end(args);
    }
    err->context = context;
    err->code    = code;
}
void
shell_error_set_errno(ShellError* err, int sys_errno, const char* context_fmt, ...) {
    shell_error_clear(err);

    char* context = nullptr;
    if (context_fmt) {
        va_list args;
        va_start(args, context_fmt);
        context = ctxstr_new(context_fmt, args);
        va_end(args);
    }
    err->context   = context;
    err->sys_errno = sys_errno;
    err->code      = SH_ERRNO;
}
void
shell_error_set_from_current_errno(ShellError* err, const char* context_fmt, ...) {
    shell_error_clear(err);
    err->sys_errno = errno;

    char* context = nullptr;
    if (context_fmt) {
        va_list args;
        va_start(args, context_fmt);
        context = ctxstr_new(context_fmt, args);
        va_end(args);
    }
    err->context = context;
    err->code    = SH_ERRNO;
}
bool
shell_error_is_set(const ShellError* err) {
    return err->code != SH_NOERR;
}

const char*
sherrstr(ShellErrorCode code, bool templated) {
    const ErrorTemplate tmp = templates[ERROR_INDEX(code)];
    return templated ? tmp.with_context : tmp.no_context;
}

char*
shell_error_message(const ShellError* err) {
    char* errstr;
    bool  templated = (err->context == nullptr) ? false : true;
    if (err->code == SH_ERRNO) {
        const char* tmp = strerror(err->sys_errno);
        errstr          = strdup(tmp);

    } else {
        const char* tmp = sherrstr(err->code, templated);
        errstr          = strdup(tmp);
    }
    size_t buffer_size;
    char*  result;
    if (templated) {
        buffer_size = (size_t) snprintf(nullptr, 0, errstr, err->context);
        char buf[buffer_size + 1];
        buf[buffer_size] = 0;
        snprintf(buf, buffer_size + 1, errstr, err->context);
        free(errstr);
        result = strdup(buf);
    } else {
        buffer_size = (size_t) snprintf(nullptr, 0, errstr);
        char buf[buffer_size + 1];
        buf[buffer_size] = 0;
        snprintf(buf, buffer_size + 1, errstr);
        free(errstr);
        result = strdup(buf);
    }
    return result;
}

void
shell_error_report(ShellError* err) {
    char* report = shell_error_message(err);
    fprintf(stderr, report);
    fflush(stderr);
    shell_error_clear(err);
    free(report);
}

int
shell_error_exit_code(const ShellError* err) {
    int result = 0;
    switch (err->code) {
    case SH_COMMAND_NOT_FOUND:
        result = 127;
        break;
    case SH_ERRNO:
        switch (err->sys_errno) {
        case EPERM:
            result = 126;
            break;
        default:
            result = 2;
        }
        break;
    case SH_INVALID_COMMAND_SUBSTITUTION:
    case SH_INVALID_IDENTIFIER:
    case SH_INVALID_EXEC_FORMAT:
    case SH_TRAILING_ESCAPE:
    case SH_TRAILING_OPERATOR:
    case SH_UNEXPECTED_TOKEN:
    case SH_UNTERMINATED_BRACES:
    case SH_UNTERMINATED_DOUBLE_QUOTE:
    case SH_UNTERMINATED_PARENTHESES:
    case SH_UNTERMINATED_SINGLE_QUOTE:
    case SH_MISSING_REDIRECTION_TARGET:
        result = 2;
        break;
    case SH_NOERR:
        result = 0;
        break;
    default:
        result = 1;
        break;
    }
    return result;
}
