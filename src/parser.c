//
// Created by nkinder on 8/13/26.
//
#include "parser.h"
#include "common.h"


#include <ctype.h>
#include <stddef.h>

#include "expand.h"
#include "lexer.h"

/* *****************************************************************************
 * Parsing helpers
 * *****************************************************************************
 */

/**
 * Parsing helper to convert a token type into an AndOrOp. Returns garbage if the
 * type doesnt correspond to an op.
 *
 * @param type Token type
 * @return AndOrOp corresponding to that type
 */
AndOrOp
andor_from_tokentype(TokenType type) {
    AndOrOp result = -1;
    switch (type) {
    case TOK_AND:
        result = AND_AND;
        break;
    case TOK_OR:
        result = AND_OR;
        break;
    }
    return result;
}

/**
 * A vector for holding argument strings during parsing. The actual output from parsing should be
 * an array which only makes room for the number of elements. However, since we don't necessarily know
 * ahead of time how many arguments there might be, a dynamic, resizable array will store them in the
 * meantime.
 */
typedef struct ArgVector {
    size_t size;
    size_t capacity;
    char** arr;
} ArgVector;

/**
 * Create a new, empty argument vector. The initial capacity is set to 16.
 *
 * @return A new vector, or null
 */
ArgVector*
av_new() {
    ArgVector* result = malloc(sizeof(ArgVector));
    if (!result) {
        return nullptr;
    }
    char** arr = malloc(sizeof(char*) * 16);
    if (!arr) {
        free(result);
        return nullptr;
    }
    for (int i = 0; i < 16; ++i) {
        arr[i] = nullptr;
    }

    result->arr      = arr;
    result->capacity = 16;
    result->size     = 0;
    return result;
}

/**
 * Delete the given vector.
 *
 * @param vec Vector to delete
 */
void
av_delete(ArgVector* vec) {
    for (int i = 0; i < vec->capacity; ++i) {
        if (vec->arr[i] != nullptr) {
            free(vec->arr[i]);
        }
    }
    free(vec->arr);
    free(vec);
}

/**
 * Append the arg to the given vector, duplicating the given string.
 *
 * @param vec Vector
 * @param arg The value to append
 */
void
av_append(ArgVector* vec, const char* arg) {
    if (vec->size == vec->capacity) {
        char** arr = realloc(vec->arr, vec->capacity * 2);
        if (arr != vec->arr) {
            vec->arr = arr;
        }
        vec->capacity *= 2;
    }

    char* argcpy          = strdup(arg);
    vec->arr[vec->size++] = argcpy;
}

/**
 * A vector for holding redirection objects during parsing. The actual output from parsing should be
 * an array which only makes room for the number of elements. However, since we don't necessarily know
 * ahead of time how many redirections there might be, a dynamic, resizable array will store them in the
 * meantime.
 */
typedef struct RedirVec {
    size_t    size;
    size_t    capacity;
    Redirect* arr;
} RedirVec;

/**
 * Allocate a new redirect vector. The underlying array is nulled as well.
 *
 * @return A new vector or nullptr
 */
RedirVec*
rv_new() {
    RedirVec* result = malloc(sizeof(RedirVec));
    if (!result) {
        return nullptr;
    }
    Redirect* arr = calloc(16, sizeof(Redirect));
    if (!arr) {
        free(result);
        return nullptr;
    }
    result->capacity = 16;
    result->size     = 0;
    result->arr      = arr;
    return result;
}

/**
 * Delete the given vector.
 *
 * @param vec Vector to delete
 */
void
rv_delete(RedirVec* vec) {
    for (int i = 0; i < vec->size; ++i) {
        free(vec->arr[i].target);
    }
    free(vec->arr);
    free(vec);
}

/**
 * Append a redirect to the vector, expanding if necessary.
 *
 * @param vec Vector
 * @param redirect Redirect to append
 */
void
rv_append(RedirVec* vec, Redirect redirect) {
    if (vec->size == vec->capacity) {
        Redirect* arr = realloc(vec->arr, vec->capacity * 2);
        if (arr != vec->arr) {
            vec->arr = arr;
        }
        vec->capacity *= 2;
        memset(vec->arr + vec->size, 0, sizeof(Redirect) * (vec->capacity - vec->size));
    }
    vec->arr[vec->size++] = redirect;
}

/**
 * Helper function for determining if a token is a redirect token or not.
 *
 * @param type Token type
 * @return True if its a redirection
 */
bool
is_redirect(TokenType type) {
    bool result = false;
    switch (type) {
    case TOK_REDIR_APPEND:
    case TOK_REDIR_HEREDOC:
    case TOK_REDIR_IN:
    case TOK_REDIR_OUT:
        result = true;
    }
    return result;
}

/**
 * Helper function to convert a TokenType into a RedirMode. If the given type is not
 * a redirection token type, an error value of REDIR_ERR will be returned.
 *
 * @param type Token type
 * @return The corresponding redir mode, or an error value
 */
RedirMode
from_redir_tok(TokenType type) {
    RedirMode result;
    switch (type) {
    case TOK_REDIR_IN:
        result = REDIR_IN;
        break;
    case TOK_REDIR_OUT:
        result = REDIR_OUT;
        break;
    case TOK_REDIR_APPEND:
        result = REDIR_APPEND;
        break;
    case TOK_REDIR_HEREDOC:
        result = REDIR_HEREDOC;
        break;
    default:
        result = 0b100;
        break;
    }
    return result;
}

/* *****************************************************************************
 * Parsing functions
 * *****************************************************************************
 */

List* NULLABLE
lex_and_parse(const char* input) {
    CharStream* stream = cs_new(input);
    if (!stream) {
        return nullptr;
    }

    TokenList* token_list = tokenlist_new_empty();
    if (!token_list) {
        cs_delete(stream);
        return nullptr;
    }

    List* result;
    int   tokenize_result = lx_tokenize(token_list, stream);
    if (tokenize_result != 0) {
        fprintf(stderr, "Error tokenizing given input. Aborting!\n");
        tokenlist_delete(token_list);
        cs_delete(stream);
        return nullptr;
    }

    TokenStream* ts = ts_new(token_list);
    if (!ts) {
        // TODO: ERROR
        tokenlist_delete(token_list);
        cs_delete(stream);
        return nullptr;
    }

    result = parse_list(ts);
    if (!result) {
        // TODO: ERR
        ts_delete(ts);
        tokenlist_delete(token_list);
        cs_delete(stream);
        return nullptr;
    }

    return result;
}

List*
parse_list(TokenStream* stream) {
    List*  list  = list_new_empty();
    AndOr* first = parse_and_or(stream);
    list->head   = le_new(first, SEP_NONE);
    list->count++;

    ListElement* tail = list->head;
    while (ts_check(stream, TOK_SEMI) || ts_check(stream, TOK_AMP)) {
        ListSep sep = ts_check(stream, TOK_SEMI) ? SEP_SEMI : SEP_AMP;
        ts_read(stream);
        tail->sep = sep;

        if (ts_at_end(stream) || ts_check(stream, TOK_EOF) || ts_check(stream, TOK_NEWLINE)) {
            break;
        }

        AndOr* next = parse_and_or(stream);
        tail->next  = le_new(next, SEP_NONE);
        tail        = tail->next;
        list->count++;
    }
    return list;
}

AndOr*
parse_and_or(TokenStream* stream) {
    AndOr*    ao       = ao_new_empty();
    Pipeline* first    = parse_pipeline(stream);
    ao->head           = aoe_new(first, AND_NONE);
    ao->count          = 1;
    AndOrElement* tail = ao->head;
    while (ts_check(stream, TOK_AND) || ts_check(stream, TOK_OR)) {
        AndOrOp op = ts_check(stream, TOK_AND) ? AND_AND : AND_OR;
        ts_read(stream);

        tail->op = op;

        Pipeline* next = parse_pipeline(stream);
        tail->next     = aoe_new(next, AND_NONE);
        ao->count++;
        tail = tail->next;
    }
    return ao;
}

Pipeline*
parse_pipeline(TokenStream* stream) {
    Pipeline* pipeline = pipeline_new_empty();
    Command*  first    = parse_command(stream);
    pipeline->head     = ple_new(first);
    if (pipeline->head != nullptr) {
        pipeline->size++;
    }

    PipelineElement* tail = pipeline->head;
    while (ts_match(stream, TOK_PIPE)) {
        Command* command = parse_command(stream);
        tail->next       = ple_new(command);
        tail             = tail->next;
        pipeline->size++;
    }
    return pipeline;
}

Command*
parse_command(TokenStream* stream) {
    Command*    result;
    ArgVector*  av              = av_new();
    RedirVec*   rv              = rv_new();
    Assignment* assignment_list = nullptr;
    while (true) {
        Token* tok = ts_peek(stream);

        if (tok->type == TOK_WORD) {
            ts_read(stream);
            av_append(av, tok->value);

        } else if (tok->type == TOK_ASSIGNMENT_WORD) {
            ts_read(stream);
            if (av->size == 0) {
                if (assignment_list == nullptr) {
                    assignment_list = ass_new(tok->value);
                } else {
                    ass_append_str(assignment_list, tok->value);
                }
            } else {
                av_append(av, tok->value);
            }
        } else if (is_redirect(tok->type)) {
            ts_read(stream);
            int       fd     = tok->fd;
            RedirMode mode   = from_redir_tok(tok->type);
            Token*    target = ts_expect(stream, TOK_WORD);
            Redirect  redir  = (Redirect){.fd = fd, .mode = mode, .target = strdup(target->value)};
            rv_append(rv, redir);
        } else {
            /* This branch means that we have encountered a token which needs to be passed back up. */

            /* Copying over the argument vector into a sized array. */
            const char** argv = malloc(sizeof(const char*) * av->size + 1);
            size_t       argc = av->size;
            for (int i = 0; i < av->size; ++i) {
                argv[i] = strdup(av->arr[i]);
            }
            argv[argc] = nullptr;
            av_delete(av);

            /* Copying the redirection vector into a sized array. */
            Redirect* rvs  = malloc(sizeof(Redirect) * rv->size);
            size_t    nrvs = rv->size;
            for (int i = 0; i < nrvs; ++i) {
                Redirect* redir = redir_new(rv->arr[i].fd, rv->arr[i].mode, rv->arr[i].target);
                rvs[i]          = *redir;
            }

            result = command_new(argv, assignment_list, nrvs, rvs);
            break;
        }
    }
    return result;
}

void
prepare_args(char** dest, TokenList* words) {
    Token* iter = words->head;
    for (int i = 0; i < words->size; ++i) {
        memcpy(dest[i], iter->value, strlen(iter->value) + 1);
        iter = iter->next;
    }
    dest[words->size] = nullptr;
}


bool
is_separator(TokenType type) {
    bool result = false;
    switch (type) {
    case TOK_SEMI:
    case TOK_AMP:
        result = true;
    }
    return result;
}

size_t
count_separators(TokenList* tokens) {
    size_t count = 0;
    Token* iter  = tokens->head;
    while (iter != nullptr) {
        if (is_separator(iter->type)) {
            ++count;
        }
        iter = iter->next;
    }
    return count;
}

ListSep
sep_from_tokentype(TokenType type) {
    ListSep result = -1;
    switch (type) {
    case TOK_SEMI:
        result = SEP_SEMI;
        break;
    case TOK_AMP:
        result = SEP_AMP;
        break;
    }
    return result;
}

bool
split_on_separator(TokenList* dst, TokenList* src) {
    Token* iter = src->head;
    Token* prev = nullptr;
    while (iter != nullptr) {
        if (is_separator(iter->type)) {
            Token* tmp = prev;
            prev->next = nullptr;
            dst->head  = tmp;
            src->size  = tokenlist_count(src);
            dst->size  = tokenlist_count(dst);
            return true;
        }
        prev = iter;
        iter = iter->next;
    }
    return false;
}


// List*
// parse_list(TokenList* tokens) {
//   size_t nandors = count_separators(tokens);
//   AndOr** andors = malloc(sizeof(AndOr*) * nandors);
//   ListSep ops[nandors];
//   ListElement elements[nandors];
//   List* result = nullptr;
//   TokenList* cpy = tokenlist_copyof(tokens);
//   if (nandors > 1) {
//     TokenList** lists = malloc(sizeof(TokenList*) * nandors);
//     lists[0] = cpy;
//     for (int i = 1; i < nandors; ++i) {
//       lists[i] = tokenlist_new_empty();
//       split_on_separator(lists[i], lists[i - 1]);
//       andors[i - 1] = parse_and_or(lists[i - 1]);
//       if (i == 1) {
//         elements[0] = (ListElement){.and_or = andors[i - 1], .sep = SEP_NONE};
//       } else {
//         elements[i - 1] = (ListElement){.and_or = andors[i - 1], .sep = sep_from_tokentype(lists[i -
//         1]->head->type)}; Token* tmp = lists[i - 1]->head; lists[i - 1]->head = lists[i-1]->head->next; lists[i -
//         1]->size--; token_delete(tmp);
//       }
//     }
//     elements[nandors - 1] = (ListElement){.and_or = andors[nandors - 1], sep}
//
//   }
// }

AndOrElement*
aoe_new(Pipeline* pipeline, AndOrOp op) {
    AndOrElement* result = malloc(sizeof(AndOrElement));
    if (!result) {
        return nullptr;
    }

    result->next     = nullptr;
    result->pipeline = pipeline;
    result->op       = op;
    return result;
}
void
aoe_delete(AndOrElement* aoe) {
    pipeline_delete(aoe->pipeline);
    free(aoe);
}
AndOr*
ao_new_empty() {
    AndOr* result = malloc(sizeof(AndOr));
    if (!result) {
        return nullptr;
    }
    result->head  = nullptr;
    result->count = 0;
    return result;
}
void
ao_delete(AndOr* ao) {
    AndOrElement* iter = ao->head;
    while (iter != nullptr) {
        AndOrElement* tmp = iter;
        iter              = iter->next;
        aoe_delete(tmp);
    }
    free(ao);
}
bool
split_on_pipes(TokenList* dest, TokenList* src) {
    Token* iter     = src->head;
    Token* prev     = nullptr;
    size_t new_size = 0;
    while (iter != nullptr) {
        if (strcmp(iter->value, "|") == 0) {
            prev->next      = nullptr;
            Token* tmp      = iter;
            size_t old_size = src->size;
            src->size       = tokenlist_count(src);
            dest->head      = tmp->next;
            dest->size      = tokenlist_count(dest);
            token_delete(tmp);
            return true;
        }
        ++new_size;
        prev = iter;
        iter = iter->next;
    }
    return false;
}

size_t
count_pipes(TokenList* tokens) {
    Token* iter  = tokens->head;
    size_t count = 0;
    while (iter != nullptr) {
        if (strcmp(iter->value, "|") == 0) {
            ++count;
        }
        iter = iter->next;
    }
    return count;
}

bool
is_ao_operator(TokenType type) {
    bool result = false;
    switch (type) {
    case TOK_AND:
    case TOK_OR:
        result = true;
    }
    return result;
}

size_t
count_operators(TokenList* tokens) {
    Token* iter      = tokens->head;
    size_t chain_len = 0;
    while (iter != nullptr) {
        if (is_ao_operator(iter->type)) {
            ++chain_len;
        }
        iter = iter->next;
    }
    return chain_len;
}

bool
split_on_operator(TokenList* dst, TokenList* src) {
    Token* iter = src->head;
    Token* prev = nullptr;
    while (iter != nullptr) {
        if (is_ao_operator(iter->type)) {
            prev->next = nullptr;
            dst->head  = iter;
            src->size  = tokenlist_count(src);
            dst->size  = tokenlist_count(dst);
            return true;
        }
        prev = iter;
        iter = iter->next;
    }
    return false;
}


// AndOr*
//   parse_and_or(TokenList* tokens) {
//   size_t npipelines = count_operators(tokens) + 1;
//   Pipeline** pipelines = malloc(sizeof(Pipeline*) * npipelines);
//   AndOrOp ops[npipelines];
//   AndOrElement elements[npipelines];
//   AndOr* result = nullptr;
//   TokenList* cpy = tokenlist_copyof(tokens);
//   if (npipelines > 1) {
//       TokenList** lists = malloc(sizeof(TokenList*) * npipelines);
//       lists[0] = cpy;
//     for (int i = 1; i < npipelines; ++i) {
//       lists[i] = tokenlist_new_empty();
//       split_on_operator(lists[i], lists[i - 1]);
//       pipelines[i - 1] = parse_pipeline(lists[i - 1]);
//       if (i == 1) {
//         elements[0] = (AndOrElement){.pipeline = pipelines[i - 1], .op = AND_NONE};
//       } else {
//         elements[i - 1] = (AndOrElement){.pipeline = pipelines[i - 1], .op = andor_from_tokentype(lists[i -
//         1]->head->type)}; Token* tmp = lists[i - 1]->head; lists[i - 1]->head = tmp->next; lists[i - 1]->size--;
//         token_delete(tmp);
//       }
//     }
//     elements[npipelines - 1] = (AndOrElement){.pipeline = pipelines[npipelines - 1], .op =
//     andor_from_tokentype(lists[npipelines - 1]->head->type)}; Token* tmp = lists[npipelines - 1]->head;
//     lists[npipelines - 1]->head = tmp->next;
//     lists[npipelines - 1]->size--;
//     token_delete(tmp);
//     result = ao_new(npipelines, elements);
//     free(pipelines);
//     free(lists);
//     tokenlist_delete(cpy);
//   } else {
//     Pipeline* pipeline = parse_pipeline(cpy);
//     elements[0] = (AndOrElement){.pipeline = pipeline, .op = AND_NONE};
//     result = ao_new(npipelines, elements);
//     free(pipelines);
//     tokenlist_delete(cpy);
//   }
//   return result;
// }

// Pipeline*
// parse_pipeline(TokenList* tokens) {
//     size_t    ncmds  = count_pipes(tokens) + 1;
//     Command** cmds   = malloc(sizeof(Command*) * ncmds);
//     TokenList* cpy    = tokenlist_copyof(tokens);
//     Pipeline* result = nullptr;
//     if (ncmds > 1) {
//         TokenList** lists = malloc(sizeof(TokenList*) * ncmds);
//         lists[0]         = cpy;
//         for (int i = 1; i < ncmds; ++i) {
//             lists[i] = tokenlist_new_empty();
//             split_on_pipes(lists[i], lists[i - 1]);
//             cmds[i - 1] = parse_command(lists[i - 1]);
//         }
//         cmds[ncmds - 1] = parse_command(lists[ncmds - 1]);
//         result          = pipeline_new(ncmds, cmds);
//         free(cmds);
//         tokenlist_delete(cpy);
//         free(lists);
//     } else {
//         cmds[0] = parse_command(cpy);
//         result  = pipeline_new(1, cmds);
//         free(cmds);
//         tokenlist_delete(cpy);
//     }
//     return result;
// }
/*

bool
is_redir(const char* str) {
    const char* iter = str;
    while (*iter) {
        if (*iter == '>') {
            if (iter != str) {
                if (isdigit(*(iter - 1))) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        } else {
            ++iter;
        }
    }

    return false;
}

*/
// void
// parse_redir(Redirect* dest, TokenList* words) {
//     int         fd   = 1;
//     RedirMode   mode = REDIR_OUT;
//     const char* iter = words->head->value;
//     if (isdigit(*iter)) {
//         const char* start = iter;
//         do {
//             ++iter;
//         } while (isdigit(*iter));
//         char* endptr;
//         fd = (int) strtol(start, &endptr, 10);
//     }
//     if (*(iter + 1) == '>') {
//         mode = REDIR_APPEND;
//     } else if (*(iter + 1) == '\0') {
//         mode = REDIR_OUT;
//     }
//     char* target = strdup(words->head->next->value);
//     dest->fd     = fd;
//     dest->mode   = mode;
//     dest->target = target;
// }

bool
check_for_bg_token(TokenList* words) {
    Token* iter = words->head;
    Token* prev = nullptr;
    while (iter != nullptr) {
        if (strcmp(iter->value, "&") == 0) {
            prev->next = nullptr;
            token_delete(iter);
            --words->size;
            return true;
        }
        prev = iter;
        iter = iter->next;
    }
    return false;
}

// Command*
// parse_command(TokenList* words) {
//     char  argbuf[50][50];
//     char* argdest[50];
//     for (int i = 0; i < 50; ++i) {
//         argdest[i] = argbuf[i];
//     }
//     Redirect  redirs[5];
//     TokenList* wordcopy = tokenlist_copyof(words);
//
//     size_t nredirs = 0;
//
//     bool isbg = check_for_bg_token(wordcopy);
//
//     Token* iter = wordcopy->head;
//     while (iter != nullptr) {
//         if (is_redir(iter->value)) {
//             TokenList* redirected = tokenlist_from_tokens(iter);
//             wordcopy->size       = wordcopy->size - redirected->size;
//             parse_redir(&redirs[nredirs++], redirected);
//         }
//         iter = iter->next;
//     }
//     prepare_args(argdest, wordcopy);
//     Command* result = command_new(argdest, isbg, nredirs, redirs);
//     if (!result) {
//         for (int i = 0; i < nredirs; ++i) {
//             free(redirs[i].target);
//         }
//         tokenlist_delete(wordcopy);
//         return nullptr;
//     }
//     tokenlist_delete(wordcopy);
//     return result;
// }


size_t
next_token(char* dest, char* input, QuoteFlagE* flag) {
    char* i       = input;
    int   destidx = 0;


    while (*i) {
        char c = *i;
        if (*flag == UNQUOTED || *flag == DOUBLE_QUOTED) {

            if (c == '\\') {
                dest[destidx++] = *i++;
                if (*i == '\0') {
                    break;
                }
                if (*flag == UNQUOTED || strchr("\\$`\"\n", *i)) {
                    dest[destidx++] = *i++;
                    continue;
                }
                dest[destidx++] = '\\';
                continue;
            }
        }
        if (c == '\'') {
            switch (*flag) {
            case SINGLE_QUOTED:
                *flag           = UNQUOTED;
                dest[destidx++] = c;
                break;

            case UNQUOTED:
                *flag           = SINGLE_QUOTED;
                dest[destidx++] = c;
                break;

            case DOUBLE_QUOTED:
                dest[destidx++] = c;
                break;
            }
            ++i;
        } else if (c == '"') {
            switch (*flag) {
            case SINGLE_QUOTED:
                dest[destidx++] = c;
                break;

            case UNQUOTED:
                *flag           = DOUBLE_QUOTED;
                dest[destidx++] = c;
                break;

            case DOUBLE_QUOTED:
                *flag           = UNQUOTED;
                dest[destidx++] = c;
                break;
            }
            ++i;
        } else if (*flag == UNQUOTED && (c == ' ' || c == '\n')) {
            if (c == ' ') {
                do {
                    //++destidx;
                    ++i;
                } while (*i == ' ');
            }
            break;
        } else {
            dest[destidx++] = c;
            ++i;
        }
    }

    // while (*i != '\0' && *i != ' ' && *i != '\n' && count < 1024 - 1) {
    //   dest[count] = *i;
    //   ++i;
    //   ++count;
    // }
    if (destidx == 0) {
        return 0;
    }

    dest[destidx] = '\0';
    return (size_t) (i - input);
}

int
exppass(TokenList* list) {
    Token* iter = list->head;

    for (int i = 0; i < list->size; ++i) {
        const char* s        = iter->value;
        const char* expanded = exptok(s);
        iter->value          = expanded;

        free((void*) s);
        iter = iter->next;
    }
    return 0;
}

void
remove_blanks(TokenList* list) {
    Token* iter = list->head;
    Token* prev = nullptr;
    while (iter != nullptr) {
        if (strlen(iter->value) == 0) {
            if (prev != nullptr) {
                prev->next = iter->next;
                list->size--;
                token_delete(iter);
                iter = prev;
            } else {
                list->head = iter->next;
                list->size--;
                token_delete(iter);
                iter = list->head;
                continue;
            }
        }
        prev = iter;
        iter = iter->next;
    }
}

TokenList*
tokenize_path(const char* path) {
    return nullptr;
}
TokenList*
tokenize_input(const char* input) {
    return nullptr;
}

// Token* token(char** out, char** in, QuoteFlagE* flag) {
//   return nullptr;
// }
// TokenList*
// tokenize_input(const char* input) {
//     QuoteFlagE  flag      = UNQUOTED;
//     char        buf[1024] = {};
//     const char* iter      = input;
//     TokenList*   result    = tokenlist_new_empty();
//
//     size_t readchars;
//
//     do {
//
//         readchars = next_token(buf, iter, &flag);
//         if (readchars > 0) {
//             size_t jumpsize = readchars;
//             iter += jumpsize;
//             tokenlist_append(result, buf);
//             memset(buf, 0, readchars);
//         }
//
//     } while (readchars != 0);
//     if (flag == SINGLE_QUOTED) {
//         tokenlist_delete(result);
//         fprintf(stderr, "syntax error: unterminated quote\n");
//         return nullptr;
//     }
//
//     exppass(result);
//     remove_blanks(result);
//     return result;
// }
//
// TokenList*
// tokenize_path(const char* path) {
//     TokenList* result = tokenlist_new_empty();
//     char*     tok    = calloc((strlen(path) + 1), sizeof(char));
//
//     const char* end            = path + strlen(path);
//     const char* iter           = path;
//     ptrdiff_t   bytesremaining = end - path;
//     while (!(iter >= end)) {
//         memset(tok, '\0', strlen(tok));
//         memccpy(tok, iter, ':', bytesremaining);
//         tok[strcspn(tok, ":")] = '\0';
//         tokenlist_append(result, tok);
//         bytesremaining = end - iter;
//         iter           = iter + strlen(tok) + 1;
//     }
//
//     free(tok);
//     return result;
// }
//
// TokenType token(char** buf, char** iter_in, QuoteFlagE* flag) {
//   char* begin = *iter_in;
//
// }
void
assignment_split(char* dest[2], const char* assignment) {
    const char *name_begin = assignment, *name_end = assignment, *value_begin = assignment, *value_end = assignment;
    const char* i = assignment;
    while (*i != '\0' && *i != '=') {
        ++i;
    }
    if (*i == '=') {
        name_end = i++;
    }
    value_begin = i;
    while (*i != '\0') {
        ++i;
    }
    value_end = i;

    size_t name_len   = (size_t) (name_end - name_begin);
    size_t value_len  = (size_t) (value_end - value_begin);
    dest[0]           = malloc(sizeof(char) * name_len + 1);
    dest[0][name_len] = '\0';
    memcpy(dest[0], name_begin, name_len);
    dest[1] = malloc(sizeof(char*) * value_len + 1);
    memcpy(dest[1], value_begin, value_len);
}
