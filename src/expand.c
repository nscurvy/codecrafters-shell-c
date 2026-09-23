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

size_t
exptilde(char* dest, QuoteFlagE* flag) {
    char* tilde_value;
    int   chars_wrote = 0;
    switch (*flag) {
    case UNQUOTED:
        tilde_value = getenv("HOME");
        break;
    case SINGLE_QUOTED:
    case DOUBLE_QUOTED:
        tilde_value = "~";
        break;
    default:
        return -1;
    }

    memmove(dest, tilde_value, strlen(tilde_value));
    const char* i = dest + strlen(tilde_value);
    chars_wrote   = i - dest;
    return chars_wrote;
}


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

// void
// assignment_split(char* namedest, char* valuedest, const char* assignment) {
//     const char* name_begin  = assignment;
//     const char* name_end    = strrchr(assignment, '=');
//     const char* value_begin = name_end + 1;
//     const char* value_end   = strrchr(assignment, '\0');
//     memcpy(namedest, assignment, (size_t) (name_end - name_begin));
//     memcpy(valuedest, assignment, (size_t) (value_end - value_begin));
// }

// size_t
// expand_assignment(Assignment* assignment) {
//     const char*    assignment_str = assignment->value;
//     StringBuilder* sb             = sb_new();
//     QuoteFlagE     flag           = UNQUOTED;
//
//     for (int i = 0; i < strlen(assignment_str); ++i) {
//         char c = assignment_str[i];
//         if (c == '\\') {
//             if (assignment_str[i + 1] == '$') {
//                 sb_appendc(sb, assignment_str[i]);
//                 sb_appendc(sb, assignment_str[i + 1]);
//                 ++i;
//                 continue;
//             }
//         }
//         if (flag == UNQUOTED || flag == SINGLE_QUOTED) {
//             if (c == '$') {
//                 if (assignment_str[i + 1] == '{') {
//                     ++i;
//                     ++i;
//                     const char* begin = &assignment_str[i];
//                     const char* end   = begin;
//                     while (*end != '\0' && *end != '}') {
//                         ++end;
//                         ++i;
//                     }
//                     char buf[end - begin + 1];
//                     memcpy(buf, begin, end - begin);
//                     buf[end - begin]      = '\0';
//                     const char* expansion = "";
//
//                     expansion = var_lookup(buf);
//                     if (expansion) {
//                         sb_appends(sb, expansion);
//                     }
//                 } else {
//                     StringBuilder* tmp = sb_new();
//                     ++i;
//                     while (true) {
//                         sb_appendc(tmp, assignment_str[i++]);
//                         const char* expansion = var_lookup(sb->str);
//                         if (expansion) {
//                             sb_appends(sb, expansion);
//                             sb_delete(tmp);
//                             break;
//                         }
//                     }
//                 }
//             } else {
//                 sb_appendc(sb, c);
//             }
//         }
//     }
//     assignment->value = sb_takestring(sb);
//     free(assignment_str);
//     sb_delete(sb);
//     return strlen(assignment->value);
// }


size_t
expvar(char* dest, const char** word, QuoteFlagE* flag) {
    size_t chars_wrote   = 0;
    char*  variable_name = malloc(sizeof(char) * strlen(*word));
    memcpy(variable_name, &(*word)[1], strlen(*word));
    variable_name[strlen(*word)] = '\0';
    const char* result           = var_lookup(variable_name);
    if (result == nullptr) {
        *word += strlen(variable_name);
        free(variable_name);
        return 0;
    }
    memcpy(dest, result, strlen(result));
    const char* i = dest + strlen(result);
    chars_wrote   = i - dest;
    *word += strlen(variable_name);
    free(variable_name);
    return chars_wrote;
}

size_t
expvar_braced(char* dest, const char** iter, QuoteFlagE* flag) {
    const char* beginning = *iter;
    *iter += 2;
    size_t count = 0;
    while (**iter != '}') {
        ++count;
        ++*iter;
    }
    char* variable_name = malloc(sizeof(char) * count + 1);
    memcpy(variable_name, &beginning[2], count);
    variable_name[count] = '\0';
    const char* result   = var_lookup(variable_name);
    if (result == nullptr) {
        free(variable_name);
        return 0;
    }
    memcpy(dest, result, strlen(result));
    const char* i = dest + strlen(result);
    free(variable_name);
    return i - dest;
}

char*
exptok(const char* word) {
    char        buf[1024] = {0};
    QuoteFlagE  flag      = UNQUOTED;
    const char* iter      = word;
    size_t      i         = 0;
    while (*iter != '\0') {
        char c = *iter;

        if (flag == UNQUOTED || flag == DOUBLE_QUOTED) {
            if (c == '\\') {
                ++iter;
                if (*iter == '\0') {
                    break;
                }
                if (flag == UNQUOTED || strchr("\\$`\"\n", *iter)) {
                    buf[i++] = *iter++;
                    continue;
                }
                buf[i++] = '\\';
                continue;
            }
        }
        if (flag == UNQUOTED) {
            switch (c) {
            case '\'':
                flag = SINGLE_QUOTED;
                break;
            case '"':
                flag = DOUBLE_QUOTED;
                break;
            case '~':
                i += exptilde(&buf[i], &flag);
                break;
            case '$':
                if (*(iter + 1) == '{') {
                    i += expvar_braced(&buf[i], &iter, &flag);
                } else {
                    i += expvar(&buf[i], &iter, &flag);
                }
                break;
            default:
                buf[i++] = c;
                break;
            }
        } else if (flag == SINGLE_QUOTED) {
            switch (c) {
            case '\'':
                flag = UNQUOTED;
                break;
            case '"':
                buf[i++] = c;
                break;
            case '~':
                i += exptilde(&buf[i], &flag);
                break;
            default:
                buf[i++] = c;
                break;
            }
        } else if (flag == DOUBLE_QUOTED) {
            switch (c) {
            case '\'':
                buf[i++] = c;
                break;
            case '"':
                flag = UNQUOTED;
                break;
            case '~':
                i += exptilde(&buf[i], &flag);
                break;
            default:
                buf[i++] = c;
                break;
            }
        }
        ++iter;
    }
    char* ret = calloc(strlen(buf) + 1, sizeof(char));
    if (!ret) {
        return nullptr;
    }

    strcpy(ret, buf);

    return ret;
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
