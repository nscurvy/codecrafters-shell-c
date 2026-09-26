//
// Created by nkinder on 8/13/26.
//
#define REFACTORING_OUT

#include "expand.h"
#include "common.h"
#include "exec.h"

#include "hashtable.h"
#include "lexer.h"
#include "parser.h"
#include "stringbuilder.h"
#include "vars.h"

struct HashTable* variable_table = nullptr;


void
expand_tilde(StringBuilder* sb) {
    const char* tilde_value = var_lookup("HOME");
    if (tilde_value) {
        sb_appends(sb, tilde_value);
    }
}

void
expand_braced_variable(const char** i, StringBuilder* sb) {
    StringBuilder* tmp = sb_new();
    while (**i != '}' && **i != '\0') {
        sb_appendc(tmp, **i);
        (*i)++;
    }
    if (**i == '}') {
        (*i)++;
    }
    const char* varname = sb_takestring(tmp);
    sb_delete(tmp);
    const char* val = var_lookup(varname);
    if (val) {
        sb_appends(sb, val);
    }
    free(varname);
}


void
expand_loose_variable(const char** i, StringBuilder* sb) {
    StringBuilder* tmp   = sb_new();
    size_t         count = 0;
    while (**i != '\0' && **i != ' ') {
        sb_appendc(tmp, **i);
        ++count;
        const char* val = var_lookupn(tmp->str, count);
        if (val) {
            (*i)++;
            sb_appends(sb, val);
            sb_delete(tmp);
            return;
        }
        (*i)++;
    }
}


void
expand_variable(const char** i, StringBuilder* sb) {
    (*i)++;
    if (**i == '{') {
        (*i)++;
        expand_braced_variable(i, sb);
    } else {
        expand_loose_variable(i, sb);
    }
}

char*
capture_list_output(List* body) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        return strdup("");
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return strdup("");
    }
    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execute_list(body);
        _exit(0);
    }
    close(pipefd[1]);
    StringBuilder* sb = sb_new();
    char           buf[4096];
    ssize_t        n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        sb_appends(sb, buf);
    }
    close(pipefd[0]);
    int status;
    waitpid(pid, &status, 0);
    char* result = (char*) sb_takestring(sb);

    sb_delete(sb);

    size_t len = strlen(result);
    while (len > 0 && result[len - 1] == '\n') {
        result[--len] = '\0';
    }

    return result;
}

void
expand_cmdsub(const char** i, StringBuilder* sb) {
    *i += 2;
    const char* begin = *i;
    const char* end   = *i;
    while (**i != ')' && **i != '\0') {
        (*i)++;
    }
    if (**i == ')') {
        end        = *i;
        size_t len = (size_t) (end - begin);
        char   buf[len + 1];
        buf[len] = '\0';
        memcpy(buf, begin, len);
        List* list   = lex_and_parse(buf);
        char* result = capture_list_output(list);
        sb_appends(sb, result);
        free(result);
        (*i)++;
    }
}

void
expand_dollarsign(const char** i, StringBuilder* sb) {
    switch (*(*i + 1)) {
    case '(':
        expand_cmdsub(i, sb);
        break;
    default:
        expand_variable(i, sb);
        break;
    }
}

const char*
expand_word(const char* str) {
    StringBuilder* sb   = sb_new_sized(strlen(str));
    QuoteFlagE     flag = UNQUOTED;

    const char* i = str;
    while (*i != '\0') {
        switch (flag) {
        case UNQUOTED:
            switch (*i) {
            case '~':
                if (i == str) {
                    expand_tilde(sb);
                } else {
                    sb_appendc(sb, *i);
                }
                break;
            case '$':
                expand_dollarsign(&i, sb);
                goto NOINC;
                break;
            case '\\':
                ++i;
                sb_appendc(sb, *i);
                break;
            case '\'':
                flag = SINGLE_QUOTED;
                break;
            case '"':
                flag = DOUBLE_QUOTED;
                break;
            default:
                sb_appendc(sb, *i);
                break;
            }

            break;
        case SINGLE_QUOTED:
            switch (*i) {
            case '\'':
                flag = UNQUOTED;
                break;
            default:
                sb_appendc(sb, *i);
            }
            break;
        case DOUBLE_QUOTED:
            switch (*i) {
            case '"':
                flag = UNQUOTED;
                break;
            case '\\':
                ++i;
                sb_appendc(sb, *i);
                break;
            case '$':
                expand_dollarsign(&i, sb);
                goto NOINC;
                break;
            default:
                sb_appendc(sb, *i);
                break;
            }
            break;
        }
        ++i;
NOINC:
    }
    const char* result = sb_takestring(sb);
    sb_delete(sb);
    return result;
}

void
expand_token(Token* tok) {

    const char* old;
    const char* repl;
    switch (tok->type) {
    case TOK_ASSIGNMENT_WORD:
    case TOK_WORD:
        repl       = expand_word(tok->value);
        old        = tok->value;
        tok->value = repl;
        free(old);
        break;
    }
}

const char*
expand_string(char** dst, const char* src) {
    StringBuilder* sb   = sb_new();
    const char*    iter = src;
    while (*iter != '\0') {
        if (*iter == '\\') {
            if (*(iter + 1) == '$') {
                iter += 2;
                continue;
            }
        } else if (*iter == '$') {
            if (*(iter + 1) == '{') {
                const char* start = iter;
                const char* end   = start + 1;
                while (*end != '\0' && *end != '}') {
                    ++end;
                }
                char* varname = calloc((unsigned long) (end - start + 1), sizeof(char));
                strncpy(varname, start, (unsigned long) (end - start));
                const char* lookup = var_lookup(varname);
                if (varname) {
                    sb_appends(sb, lookup);
                }
            } else {
                const char* start = iter;
                const char* end   = start + 1;
                while (true) {
                    char* tmp = calloc((unsigned long) (end - start + 1), sizeof(char));
                    strncpy(tmp, start, (unsigned long) (end - start));
                    const char* lookup = var_lookup(tmp);
                    if (lookup) {
                        sb_appends(sb, lookup);
                        free(tmp);
                        break;
                    }

                    ++end;
                    free(tmp);
                }
            }
        } else {
            sb_appendc(sb, *iter);
        }
        ++iter;
    }
    *dst               = (char*) sb_takestring(sb);
    const char* result = *dst;
    sb_delete(sb);
    return result;
}


char*
expand_home(char* buf, const char* path) {
    char* buff_iter = buf;
    char* homepath  = getenv("HOME");

    memmove(buff_iter, homepath, strlen(homepath));
    buff_iter += strlen(homepath);
    *buff_iter = '/';
    ++buff_iter;
    strncat(buff_iter, &path[1], strlen(&path[1]));
    buff_iter += strlen(&path[1]) + 1;

    return buf;
}
