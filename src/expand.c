//
// Created by nkinder on 8/13/26.
//
#define REFACTORING_OUT

#include "common.h"
#include "expand.h"

#include "parser.h"
#include "declare.h"

struct HashTable* variable_table = nullptr;

size_t
exptilde(char *dest, QuoteFlagE *flag) {
    char *tilde_value;
    int chars_wrote = 0;
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
    const char *i = dest + strlen(tilde_value);
    chars_wrote = i - dest;
    return chars_wrote;
}

size_t expvar(char* dest, const char** word, QuoteFlagE* flag) {
    size_t chars_wrote = 0;
    char* variable_name = malloc(sizeof(char) * strlen(*word));
    memcpy(variable_name, &(*word)[1], strlen(*word));
    variable_name[strlen(*word)] = '\0';
    const char* result = ht_get(variable_table, variable_name);
    memcpy(dest, result, strlen(result));
    const char* i = dest + strlen(result);
    *word += strlen(variable_name);
    chars_wrote = i - dest;
    free(variable_name);
    return chars_wrote;
}

size_t expvar_braced(char* dest, const char** iter, QuoteFlagE* flag) {
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
    const char* result = ht_get(variable_table, variable_name);
    memcpy(dest, result, strlen(result));
    const char* i = dest + strlen(result);
    free(variable_name);
    return i - dest;
}

char *
exptok(const char *word) {
    char buf[1024] = {0};
    QuoteFlagE flag = UNQUOTED;
    const char *iter = word;
    size_t i = 0;
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
    char *ret = calloc(strlen(buf) + 1, sizeof(char));
    if (!ret) {
        return nullptr;
    }

    strcpy(ret, buf);

    return ret;
}

char *
expand_home(char *buf, const char *path) {
    char *buff_iter = buf;
    char *homepath = getenv("HOME");

    memmove(buff_iter, homepath, strlen(homepath));
    buff_iter += strlen(homepath);
    *buff_iter = '/';
    ++buff_iter;
    strncat(buff_iter, &path[1], strlen(&path[1]));
    buff_iter += strlen(&path[1]) + 1;

    return buf;
}
