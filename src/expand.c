//
// Created by nkinder on 8/13/26.
//
#define REFACTORING_OUT

#include "expand.h"
#include "common.h"

#include "declare.h"
#include "lexer.h"
#include "parser.h"
#include "stringbuilder.h"

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
            const char* start = iter;
            const char* end   = start + 1;
            while (true) {
                if (variable_table == nullptr) {
                    variable_table = ht_new();
                }
                char* tmp = calloc(end - start + 1, sizeof(char));
                strncpy(tmp, start, end - start);
                if (ht_contains(variable_table, tmp)) {
                    break;
                }
            }
        }
        ++iter;
    }
}

void
assignment_split(char* namedest, char* valuedest, const char* assignment) {
    const char* name_begin  = assignment;
    const char* name_end    = strrchr(assignment, '=');
    const char* value_begin = name_end + 1;
    const char* value_end   = strrchr(assignment, '\0');
    memcpy(namedest, assignment, (size_t) (name_end - name_begin));
    memcpy(valuedest, assignment, (size_t) (value_end - value_begin));
}

size_t
expand_assignment(Assignment* assignment) {
    const char*    assignment_str = assignment->value;
    StringBuilder* sb             = sb_new();
    for (int i = 0; i < strlen(assignment_str); ++i) {
        char c = assignment_str[i];
        if (c == '\\') {
            if (assignment_str[i + 1] == '$') {
                sb_appendc(sb, assignment_str[i]);
                sb_appendc(sb, assignment_str[i + 1]);
                ++i;
                continue;
            }
        }
        if (c == '$') {
            if (assignment_str[i + 1] == '{') {
                ++i;
                ++i;
                const char* begin = &assignment_str[i];
                const char* end   = begin;
                while (*end != '\0' && *end != '}') {
                    ++end;
                    ++i;
                }
                if (variable_table == nullptr) {
                    variable_table = ht_new();
                }
                char buf[end - begin + 1];
                memcpy(buf, begin, end - begin);
                buf[end - begin]      = '\0';
                const char* expansion = "";
                if (ht_contains(variable_table, buf)) {
                    expansion = ht_get(variable_table, buf);
                }
                sb_appends(sb, expansion);
            } else {
                StringBuilder* tmp = sb_new();
                ++i;
                while (true) {
                    sb_appendc(tmp, assignment_str[i++]);
                    if (ht_contains(variable_table, sb->str)) {
                        const char* expansion = ht_get(variable_table, sb->str);
                        sb_appends(sb, expansion);
                        sb_delete(tmp);
                        break;
                    }
                }
            }
        } else {
            sb_appendc(sb, c);
        }
    }
}

size_t
expvar(char* dest, const char** word, QuoteFlagE* flag) {
    size_t chars_wrote   = 0;
    char*  variable_name = malloc(sizeof(char) * strlen(*word));
    memcpy(variable_name, &(*word)[1], strlen(*word));
    variable_name[strlen(*word)] = '\0';
    if (variable_table == nullptr) {
        variable_table = ht_new();
    }
    const char* result = ht_get(variable_table, variable_name);
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
    if (variable_table == nullptr) {
        variable_table = ht_new();
    }
    const char* result = ht_get(variable_table, variable_name);
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
