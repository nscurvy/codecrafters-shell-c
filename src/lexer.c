//
// Created by nkinder on 9/4/26.
//

#include "lexer.h"
#include "common.h"
#include <ctype.h>
#include <limits.h>
#include <tgmath.h>
const int EOI = -1;

CharStream*
cs_new(const char* data) {
    const char* copy = strdup(data);
    if (!copy) {
        return nullptr;
    }

    CharStream* stream = (CharStream*) malloc(sizeof(CharStream));
    if (!stream) {
        free(copy);
        return nullptr;
    }

    stream->data = copy;
    stream->len  = strlen(stream->data);
    stream->pos  = 0;
    return stream;
}

void
cs_delete(CharStream* stream) {
    free(stream->data);
    free(stream);
}
int
cs_peek(CharStream* stream) {
    if (stream->pos == stream->len) {
        return EOI;
    }
    return (int) stream->data[stream->pos];
}

int
cs_peek_ahead(CharStream* stream, int n) {
    size_t target = stream->pos + (size_t) n;
    if (target >= stream->len) {
        return EOI;
    }

    return (int) stream->data[target];
}

int
cs_read(CharStream* stream) {
    if (stream->pos == stream->len) {
        return EOI;
    }

    return stream->data[stream->pos++];
}

bool
cs_eoi(CharStream* stream) {
    return cs_peek(stream) == EOI;
}

bool
cs_match(CharStream* stream, int c) {
    int ch = cs_peek(stream);
    if (ch == c) {
        cs_read(stream);
        return true;
    }
    return false;
}

void
cs_reset(CharStream* stream) {
    stream->pos = 0;
}

static CleanupPolicy cleanup_policy = LX_CLEANUP;

void
set_cleanup_policy(CleanupPolicy policy) {
    cleanup_policy = policy;
}

bool
isoperator(int c) {
    switch (c) {
    case '|':
    case '&':
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case '<':
    case '>':
        return true;
    default:
        return false;
    }
}

TokenType
lx_scan_operator(CharStream* stream, int c, char** end) {
    TokenType result = TOK_WORD;
    switch (c) {
    case '|':
        if (cs_peek_ahead(stream, 1) == '|') {
            ++(*end);
            cs_read(stream);
            result = TOK_OR;
        } else {
            result = TOK_PIPE;
        }
        break;
    case '&':
        if (cs_peek_ahead(stream, 1) == '&') {
            ++(*end);
            cs_read(stream);
            result = TOK_AND;
        } else {
            result = TOK_AMP;
        }
        break;
    case ';':
        result = TOK_SEMI;
        break;
    case '<':
        if (cs_peek_ahead(stream, 1) == '<') {
            ++(*end);
            cs_read(stream);
            result = TOK_REDIR_HEREDOC;
        } else {
            result = TOK_REDIR_IN;
        }
        break;
    case '>':
        if (cs_peek_ahead(stream, 1) == '>') {
            ++(*end);
            cs_read(stream);
            result = TOK_REDIR_APPEND;
        } else {
            result = TOK_REDIR_OUT;
        }
        break;
    case '(':
        result = TOK_LPAREN;
        break;
    case ')':
        result = TOK_RPAREN;
        break;
    case '{':
        result = TOK_LBRACE;
        break;
    case '}':
        result = TOK_RBRACE;
        break;
    }
    cs_read(stream);
    ++(*end);
    return result;
}

void
toggle_flag(int c, QuoteFlagE* flag) {
    switch (*flag) {
    case UNQUOTED:
        switch (c) {
        case '\'':
            *flag = SINGLE_QUOTED;
            break;
        case '\"':
            *flag = DOUBLE_QUOTED;
            break;
        }
        break;
    case SINGLE_QUOTED:
        switch (c) {
        case '\'':
            *flag = UNQUOTED;
            break;
        }
        break;
    case DOUBLE_QUOTED:
        switch (c) {
        case '"':
            *flag = UNQUOTED;
            break;
        }
        break;
    }
}

void
consume_ws(CharStream* stream, char** begin, char** end) {
    int c = cs_peek(stream);
    while (isspace(c)) {
        cs_read(stream);
        c = cs_peek(stream);
        ++(*begin);
        ++(*end);
    }
}

TokenType
lx_scan(CharStream* stream, char** begin, char** end) {
    QuoteFlagE flag  = UNQUOTED;
    int        nextc = cs_peek(stream);
    int        c;
    *begin = &(stream->data[stream->pos]);
    *end   = *begin;
    if (isspace(nextc)) {
        if (nextc == '\n') {
            (*end)++;
            cs_read(stream);
            return TOK_NEWLINE;
        }
        consume_ws(stream, begin, end);
    }
    if (cs_peek(stream) == EOI) {
        return TOK_EOF;
    }
    TokenType predicted_type = TOK_WORD;
    do {
        c     = cs_peek(stream);
        nextc = cs_peek_ahead(stream, 1);
        if (flag == SINGLE_QUOTED) {
            if (c == '\'') {
                toggle_flag(c, &flag);
            } else if (c == EOI) {
                return TOK_ERR;
            }
        } else if (flag == DOUBLE_QUOTED) {
            if (c == '"') {
                toggle_flag(c, &flag);
            } else if (c == EOI) {
                return TOK_ERR;
            }
        } else {
            if (isoperator(c)) {
                if (*end == *begin) {
                    return lx_scan_operator(stream, c, end);
                } else {
                    return predicted_type;
                }
            } else if (c == '$' || c == '`') {
                if (c == '$' && nextc == '(') {
                    c = cs_read(stream);
                    (*end)++;
                } else if (c == '$' && nextc != '(') {
                    goto SKIP_CMDSUB_LOOP;
                }
                int depth = 1;
                c         = cs_read(stream);
                (*end)++;
                while (depth != 0) {
                    c = cs_peek(stream);
                    if (c == ')' || c == '`') {
                        --depth;
                    } else if (c == '(') {
                        ++depth;
                    } else if (c == EOI) {
                        errno = EINVAL;
                        return TOK_ERR;
                    }
                    (*end)++;
                    cs_read(stream);
                }
                nextc = cs_peek(stream);
                continue;
SKIP_CMDSUB_LOOP:

            } else if (c == '\'' || c == '"') {
                toggle_flag(c, &flag);
            } else if (c == '\\') {
                if (nextc == 'n') {
                    c = cs_read(stream);
                    (*end)++;
                    predicted_type = TOK_NEWLINE;
                    break;
                } else if (nextc == '\'' || nextc == '"') {
                    cs_read(stream);
                    (*end)++;
                    continue;
                } else if (nextc == ' ') {
                    c     = cs_read(stream);
                    nextc = cs_peek_ahead(stream, 1);
                    (*end)++;
                    (*end)++;
                    continue;
                }
            } else if (isdigit(c) && *end == *begin) {
                if (nextc == '<' || nextc == '>') {
                    cs_read(stream);
                    c = cs_peek(stream);
                    (*end)++;
                    return lx_scan_operator(stream, c, end);
                }
            }
        }
        if (c == '=') {
            predicted_type = TOK_ASSIGNMENT_WORD;
        }
        cs_read(stream);
        (*end)++;
    } while (!lx_end_of_token(nextc, flag));
    if (flag == DOUBLE_QUOTED || flag == SINGLE_QUOTED) {
        return TOK_ERR;
    }
    return predicted_type;
}

bool
is_redir(TokenType type) {
    bool result = false;
    switch (type) {
    case TOK_REDIR_APPEND:
    case TOK_REDIR_HEREDOC:
    case TOK_REDIR_IN:
    case TOK_REDIR_OUT:
        result = true;
        break;
    }
    return result;
}

Token*
lx_token(CharStream* stream) {
    char*     start = nullptr;
    char*     end   = nullptr;
    TokenType type  = lx_scan(stream, &start, &end);
    if (type == TOK_EOF) {
        return newtok(type, "");
    } else if (type == TOK_ERR) {
        errno = EINVAL;
        return nullptr;
    }
    size_t distance = end - start;
    char   arr[distance + 1];
    memcpy(arr, start, distance);
    arr[distance] = '\0';
    Token* tok;
    if (is_redir(type)) {
        errno     = 0;
        char* end = nullptr;
        long  fdl = strtol(arr, &end, 10);
        if (fdl >= INT_MAX || errno == ERANGE) {
            errno = ERANGE;
            return nullptr;
        } else if (end == arr) {
            if (type == TOK_REDIR_IN || type == TOK_REDIR_HEREDOC) {
                fdl = 0;
            } else if (type == TOK_REDIR_OUT || type == TOK_REDIR_APPEND) {
                fdl = 1;
            }
        }
        int fd = (int) fdl;
        tok    = token_new(type, arr, fd);
    } else {
        tok = newtok(type, arr);
    }
    if (!tok) {
        return nullptr;
    }
    return tok;
}

int
lx_tokenize(TokenList* dst, CharStream* in) {
    errno      = 0;
    Token* tok = nullptr;
    do {
        tok = lx_token(in);
        if (tok == nullptr) {
            perror("Failed at lexing the given input.");
            break;
        }
        tokenlist_append_tok(dst, tok);
    } while (tok->type != TOK_EOF);

    return errno;
}

bool
lx_end_of_token(int c, QuoteFlagE flag) {
    if (flag == UNQUOTED) {
        return isspace(c) || c == -1;
    } else {
        return c == -1;
    }
}
