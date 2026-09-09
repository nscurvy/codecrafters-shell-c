//
// Created by nkinder on 8/13/26.
//
#include "parser.h"
#include "common.h"


#include <ctype.h>
#include <stddef.h>

#include "expand.h"


void
prepare_args(char** dest, TokenList* words) {
    Token* iter = words->head;
    for (int i = 0; i < words->size; ++i) {
        memcpy(dest[i], iter->value, strlen(iter->value) + 1);
        iter = iter->next;
    }
    dest[words->size] = nullptr;
}

size_t
argvlen(char** argv) {
    size_t len  = 0;
    char** iter = argv;
    while (*iter != nullptr) {
        len++;
        iter++;
    }
    return ++len;
}

Command*
command_new(char** argv, bool bgjob, size_t nredirs, Redirect redirs[]) {
    size_t len  = argvlen(argv);
    char** args = malloc(sizeof(char*) * len);

    for (int i = 0; i < len; ++i) {
        if (i == len - 1) {
            args[i] = nullptr;
            break;
        }
        char* tmp = strdup(argv[i]);
        if (!tmp) {
            for (int j = 0; j < i; ++j) {
                free(args[j]);
            }
            free(args);
            return nullptr;
        } else {
            args[i] = tmp;
        }
    }

    Command* command = malloc(sizeof(Command) + (sizeof(Redirect) * nredirs));
    if (!command) {
        for (int i = 0; i < len - 1; ++i) {
            free(args[i]);
        }
        free(args);
        return nullptr;
    }
    command->bgjob = bgjob;

    command->argv    = args;
    command->nredirs = nredirs;
    memmove(command->redirs, redirs, sizeof(Redirect) * nredirs);


    return command;
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
  Token* iter = tokens->head;
  while (iter != nullptr) {
    if (is_separator(iter->type)) {
      ++count;
    }
    iter = iter->next;
  }
  return count;
}

ListSep sep_from_tokentype(TokenType type) {
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

bool split_on_separator(TokenList* dst, TokenList* src) {
  Token* iter = src->head;
  Token* prev = nullptr;
  while (iter != nullptr) {
    if (is_separator(iter->type)) {
      Token* tmp = prev;
      prev->next = nullptr;
      dst->head = tmp;
      src->size = tokenlist_count(src);
      dst->size = tokenlist_count(dst);
      return true;
    }
    prev = iter;
    iter = iter->next;
  }
  return false;
}

TokenStream*
ts_new(TokenList* tokens) {
  TokenStream* result = malloc(sizeof(TokenStream) + tokens->size * sizeof(Token*));
  if (!result) {
    return nullptr;
  }
  Token* iter = tokens->head;
  for (int i = 0; i < tokens->size; ++i) {
    result->tokens[i] = iter;
    iter = iter->next;
  }
  result->pos = 0;
  result->len = tokens->size;
  return result;
}
void
ts_delete(TokenStream* tokens) {
  free(ts->tokens);
  free(ts);
}
Token*
ts_peek(TokenStream* stream) {
  return stream->tokens[stream->pos];
}
Token*
ts_peek_ahead(TokenStream* stream, size_t n) {
  if (stream->pos + n >= stream->len) {
    return stream->tokens[stream->len - 1];
  }
  size_t jump = stream->pos + n;
  return stream->tokens[jump];
}
Token*
ts_read(TokenStream* stream) {
  if (stream->pos == stream->len - 1) {
    return stream->tokens[stream->len - 1];
  }
  return stream->tokens[stream->pos++];
}
bool
ts_check(TokenStream* stream, TokenType type) {
  if (ts_peek(stream)->type == type) {
    return true;
  }
  return false;
}
bool
ts_match(TokenStream* stream, TokenType type) {
  if (ts_peek(stream)->type == type) {
    ts_read(stream);
    return true;
  }
  return false;
}
Token*
ts_expect(TokenStream* stream, TokenType type) {
  if (!ts_check(stream, type)) {
    errno = EINVAL;
    return nullptr;
  }
  return ts_read(stream);
}
bool
ts_at_end(TokenStream* stream) {
  return stream->pos == stream->len - 1;
}

List*
list_new(size_t count, ListElement elements[]) {
  List* result = malloc(sizeof(List) + count * sizeof(ListElement));
  if (!result) {
    return nullptr;
  }
  result->count = count;
  for (int i = 0; i < count; ++i) {
    result->elements[i] = elements[i];
  }
  return result;
}

void
list_delete(List* list) {
  for (int i = 0; i < list->count; ++i) {
    ao_delete(list->elements[i].and_or);
  }
  free(list);
}

//List*
//parse_list(TokenList* tokens) {
//  size_t nandors = count_separators(tokens);
//  AndOr** andors = malloc(sizeof(AndOr*) * nandors);
//  ListSep ops[nandors];
//  ListElement elements[nandors];
//  List* result = nullptr;
//  TokenList* cpy = tokenlist_copyof(tokens);
//  if (nandors > 1) {
//    TokenList** lists = malloc(sizeof(TokenList*) * nandors);
//    lists[0] = cpy;
//    for (int i = 1; i < nandors; ++i) {
//      lists[i] = tokenlist_new_empty();
//      split_on_separator(lists[i], lists[i - 1]);
//      andors[i - 1] = parse_and_or(lists[i - 1]);
//      if (i == 1) {
//        elements[0] = (ListElement){.and_or = andors[i - 1], .sep = SEP_NONE};
//      } else {
//        elements[i - 1] = (ListElement){.and_or = andors[i - 1], .sep = sep_from_tokentype(lists[i - 1]->head->type)};
//        Token* tmp = lists[i - 1]->head;
//        lists[i - 1]->head = lists[i-1]->head->next;
//        lists[i - 1]->size--;
//        token_delete(tmp);
//      }
//    }
//    elements[nandors - 1] = (ListElement){.and_or = andors[nandors - 1], sep}
//
//  }
//}

AndOrElement*
aoe_new(Pipeline* pipeline, AndOrOp op) {
  AndOrElement* result = malloc(sizeof(AndOrElement));
  if (!result) {
    return nullptr;
  }

  result->pipeline = pipeline;
  result->op = op;
}
void
aoe_delete(AndOrElement* aoe) {
  pipeline_delete(aoe->pipeline);
  free(aoe);
}
AndOr*
ao_new(size_t count, AndOrElement elements[]) {
  AndOr* result = malloc(sizeof(AndOr) + count * sizeof(AndOrElement));
  if (!result) {
    return nullptr;
  }
  for (int i = 0; i < count; ++i) {
    result->elements[i] = elements[i];
  }
  result->count = count;
  return result;
}
void
ao_delete(AndOr* ao) {
  for (int i = 0; i < ao->count; ++i) {
    pipeline_delete(ao->elements[i].pipeline);
  }
  free(ao->elements);
  free(ao);
}
bool
split_on_pipes(TokenList* dest, TokenList* src) {
    Token* iter     = src->head;
    Token* prev     = nullptr;
    size_t    new_size = 0;
    while (iter != nullptr) {
        if (strcmp(iter->value, "|") == 0) {
            prev->next         = nullptr;
            Token* tmp      = iter;
            size_t    old_size = src->size;
            src->size          = tokenlist_count(src);
            dest->head         = tmp->next;
            dest->size         = tokenlist_count(dest);
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
    size_t    count = 0;
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
  Token* iter = tokens->head;
  size_t chain_len = 0;
  while (iter != nullptr) {
      if (is_ao_operator(iter->type)) {
        ++chain_len;
      }
    iter = iter->next;
  }
  return chain_len;
}

bool split_on_operator(TokenList* dst, TokenList* src) {
  Token* iter = src->head;
  Token* prev = nullptr;
  while (iter != nullptr) {
    if (is_ao_operator(iter->type)) {
      prev->next = nullptr;
      dst->head = iter;
      src->size = tokenlist_count(src);
      dst->size = tokenlist_count(dst);
      return true;
    }
    prev = iter;
    iter = iter->next;
  }
  return false;
}

AndOrOp andor_from_tokentype(TokenType type) {
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

AndOr*
  parse_and_or(TokenList* tokens) {
  size_t npipelines = count_operators(tokens) + 1;
  Pipeline** pipelines = malloc(sizeof(Pipeline*) * npipelines);
  AndOrOp ops[npipelines];
  AndOrElement elements[npipelines];
  AndOr* result = nullptr;
  TokenList* cpy = tokenlist_copyof(tokens);
  if (npipelines > 1) {
      TokenList** lists = malloc(sizeof(TokenList*) * npipelines);
      lists[0] = cpy;
    for (int i = 1; i < npipelines; ++i) {
      lists[i] = tokenlist_new_empty();
      split_on_operator(lists[i], lists[i - 1]);
      pipelines[i - 1] = parse_pipeline(lists[i - 1]);
      if (i == 1) {
        elements[0] = (AndOrElement){.pipeline = pipelines[i - 1], .op = AND_NONE};
      } else {
        elements[i - 1] = (AndOrElement){.pipeline = pipelines[i - 1], .op = andor_from_tokentype(lists[i - 1]->head->type)};
        Token* tmp = lists[i - 1]->head;
        lists[i - 1]->head = tmp->next;
        lists[i - 1]->size--;
        token_delete(tmp);
      }
    }
    elements[npipelines - 1] = (AndOrElement){.pipeline = pipelines[npipelines - 1], .op = andor_from_tokentype(lists[npipelines - 1]->head->type)};
    Token* tmp = lists[npipelines - 1]->head;
    lists[npipelines - 1]->head = tmp->next;
    lists[npipelines - 1]->size--;
    token_delete(tmp);
    result = ao_new(npipelines, elements);
    free(pipelines);
    free(lists);
    tokenlist_delete(cpy);
  } else {
    Pipeline* pipeline = parse_pipeline(cpy);
    elements[0] = (AndOrElement){.pipeline = pipeline, .op = AND_NONE};
    result = ao_new(npipelines, elements);
    free(pipelines);
    tokenlist_delete(cpy);
  }
  return result;
}

Pipeline*
parse_pipeline(TokenList* tokens) {
    size_t    ncmds  = count_pipes(tokens) + 1;
    Command** cmds   = malloc(sizeof(Command*) * ncmds);
    TokenList* cpy    = tokenlist_copyof(tokens);
    Pipeline* result = nullptr;
    if (ncmds > 1) {
        TokenList** lists = malloc(sizeof(TokenList*) * ncmds);
        lists[0]         = cpy;
        for (int i = 1; i < ncmds; ++i) {
            lists[i] = tokenlist_new_empty();
            split_on_pipes(lists[i], lists[i - 1]);
            cmds[i - 1] = parse_command(lists[i - 1]);
        }
        cmds[ncmds - 1] = parse_command(lists[ncmds - 1]);
        result          = pipeline_new(ncmds, cmds);
        free(cmds);
        tokenlist_delete(cpy);
        free(lists);
    } else {
        cmds[0] = parse_command(cpy);
        result  = pipeline_new(1, cmds);
        free(cmds);
        tokenlist_delete(cpy);
    }
    return result;
}

Pipeline*
pipeline_new(size_t ncmds, Command** cmds) {
    Pipeline* result = malloc(sizeof(Pipeline) + (sizeof(Command*) * ncmds));
    if (!result) {
        return nullptr;
    }

    for (int i = 0; i < ncmds; ++i) {
        result->cmds[i] = *(cmds + i);
    }
    result->ncmds = ncmds;
    return result;
}

void
pipeline_delete(Pipeline* pipeline) {
    for (int i = 0; i < pipeline->ncmds; ++i) {
        command_delete(pipeline->cmds[i]);
    }
    free(pipeline);
}

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

void
parse_redir(Redirect* dest, TokenList* words) {
    int         fd   = 1;
    RedirMode   mode = REDIR_OUT;
    const char* iter = words->head->value;
    if (isdigit(*iter)) {
        const char* start = iter;
        do {
            ++iter;
        } while (isdigit(*iter));
        char* endptr;
        fd = (int) strtol(start, &endptr, 10);
    }
    if (*(iter + 1) == '>') {
        mode = REDIR_APPEND;
    } else if (*(iter + 1) == '\0') {
        mode = REDIR_OUT;
    }
    char* target = strdup(words->head->next->value);
    dest->fd     = fd;
    dest->mode   = mode;
    dest->target = target;
}

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

Command*
parse_command(TokenList* words) {
    char  argbuf[50][50];
    char* argdest[50];
    for (int i = 0; i < 50; ++i) {
        argdest[i] = argbuf[i];
    }
    Redirect  redirs[5];
    TokenList* wordcopy = tokenlist_copyof(words);

    size_t nredirs = 0;

    bool isbg = check_for_bg_token(wordcopy);

    Token* iter = wordcopy->head;
    while (iter != nullptr) {
        if (is_redir(iter->value)) {
            TokenList* redirected = tokenlist_from_tokens(iter);
            wordcopy->size       = wordcopy->size - redirected->size;
            parse_redir(&redirs[nredirs++], redirected);
        }
        iter = iter->next;
    }
    prepare_args(argdest, wordcopy);
    Command* result = command_new(argdest, isbg, nredirs, redirs);
    if (!result) {
        for (int i = 0; i < nredirs; ++i) {
            free(redirs[i].target);
        }
        tokenlist_delete(wordcopy);
        return nullptr;
    }
    tokenlist_delete(wordcopy);
    return result;
}

void
command_delete(Command* command) {
    char** iter = command->argv;
    while (*iter != nullptr) {
        free(*iter++);
    }
    free(command->argv);
    free(command);
}

Redirect*
redir_new(int fd, RedirMode mode, const char* target) {
    Redirect* result = malloc(sizeof(Redirect));
    if (!result) {
        return nullptr;
    }

    char* redirect_target = strdup(target);
    if (!redirect_target) {
        free(result);
        return nullptr;
    }

    result->fd     = fd;
    result->mode   = mode;
    result->target = redirect_target;

    return result;
}

void
redir_delete(Redirect* redir) {
    free(redir->target);
    free(redir);
}

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

TokenList* tokenize_path(const char* path) {
  return nullptr;
}
TokenList* tokenize_input(const char* input) {
  return nullptr;
}

//Token* token(char** out, char** in, QuoteFlagE* flag) {
//  return nullptr;
//}
//TokenList*
//tokenize_input(const char* input) {
//    QuoteFlagE  flag      = UNQUOTED;
//    char        buf[1024] = {};
//    const char* iter      = input;
//    TokenList*   result    = tokenlist_new_empty();
//
//    size_t readchars;
//
//    do {
//
//        readchars = next_token(buf, iter, &flag);
//        if (readchars > 0) {
//            size_t jumpsize = readchars;
//            iter += jumpsize;
//            tokenlist_append(result, buf);
//            memset(buf, 0, readchars);
//        }
//
//    } while (readchars != 0);
//    if (flag == SINGLE_QUOTED) {
//        tokenlist_delete(result);
//        fprintf(stderr, "syntax error: unterminated quote\n");
//        return nullptr;
//    }
//
//    exppass(result);
//    remove_blanks(result);
//    return result;
//}
//
//TokenList*
//tokenize_path(const char* path) {
//    TokenList* result = tokenlist_new_empty();
//    char*     tok    = calloc((strlen(path) + 1), sizeof(char));
//
//    const char* end            = path + strlen(path);
//    const char* iter           = path;
//    ptrdiff_t   bytesremaining = end - path;
//    while (!(iter >= end)) {
//        memset(tok, '\0', strlen(tok));
//        memccpy(tok, iter, ':', bytesremaining);
//        tok[strcspn(tok, ":")] = '\0';
//        tokenlist_append(result, tok);
//        bytesremaining = end - iter;
//        iter           = iter + strlen(tok) + 1;
//    }
//
//    free(tok);
//    return result;
//}
//
//TokenType token(char** buf, char** iter_in, QuoteFlagE* flag) {
//  char* begin = *iter_in;
//
//}
