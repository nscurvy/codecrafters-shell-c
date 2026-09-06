//
// Created by nkinder on 9/4/26.
//

#include "lexer.h"
#include "common.h"
#include <ctype.h>
const int EOI = -1;

CharStream*
cs_new(const char* data) {
  const char* copy = strdup(data);
  if (!copy) {
    return nullptr;
  }

  CharStream* stream = (CharStream*)malloc(sizeof(CharStream));
  if (!stream) {
    free(copy);
    return nullptr;
  }

  stream->data = copy;
  stream->len = strlen(stream->data);
  stream->pos = 0;
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
  return (int)stream->data[stream->pos];
}

int
cs_peek_ahead(CharStream* stream, int n) {
  size_t target = stream->pos + (size_t)n;
  if (target >= stream->len) {
    return EOI;
  }

  return (int)stream->data[target];
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

void set_cleanup_policy(CleanupPolicy policy) {
  cleanup_policy = policy;
}

bool isoperator(int c) {
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
  case '`':
    return true;
  default:
    return false;
  }
}

TokenType lx_scan_operator(CharStream* stream, int c, char** end) {
  TokenType result = TOK_WORD;
  ++(*end);
  switch (c) {
  case '|':
    ++(*end);
    if (cs_match(stream, '|')) {
      ++(*end);
      result = TOK_OR;
    } else {
      result = TOK_PIPE;
    }
    break;
  case '&':
    ++(*end);
    if (cs_match(stream, '&')) {
      ++(*end);
      result = TOK_AND;
    } else {
      result = TOK_AMP;
    }
    break;
  case ';':
    result = TOK_SEMI;
    break;
  case '<':
    if (cs_match(stream, '<')) {
      ++(*end);
      result = TOK_REDIR_HEREDOC;
    } else {
      result = TOK_REDIR_IN;
    }
    break;
  case '>':
    if (cs_match(stream, '>')) {
      ++(*end);
      result = TOK_REDIR_APPEND;
    } else {
      result = TOK_REDIR_OUT;
    }
    break;
  case '`':
    result = TOK_WORD;
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
  ++(*end);
  return result;
}

void toggle_flag(int c, QuoteFlagE* flag) {
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

TokenType lx_scan(CharStream* stream, char** begin, char** end) {
  if (cs_peek(stream) == EOI) {
    return TOK_EOF;
  }
  QuoteFlagE flag = UNQUOTED;
  int nextc = cs_peek(stream);
  int c;
  *begin = &(stream->data[stream->pos]);
  *end = *begin;
  TokenType predicted_type = TOK_WORD;
  do {
    c = cs_read(stream);
    nextc = cs_peek(stream);
    if (isoperator(c)) {
      return lx_scan_operator(stream, c, end);
    } else if (c == '$') {
      if (nextc == '(') {
        nextc = cs_read(stream);
        (*end)++;
        return TOK_CMDSUB_START;
      }
    } else if (c == '\'' || c == '"') {
      toggle_flag(c, &flag);
    } else if (c == '\\') {
      if (nextc == 'n') {
        c = cs_read(stream);
        (*end)++;
        return TOK_NEWLINE;
      } else if (nextc == '\'' || nextc == '"') {
        continue;
      }
    }
    (*end)++;
  } while (!lx_end_of_token(c, flag));
  return predicted_type;
}

Token* lx_token(CharStream* stream) {
  char* start = nullptr;
  char* end = nullptr;
  TokenType type = lx_scan(stream, &start, &end);
  if (type == TOK_EOF) {
    errno = 0;
    return nullptr;
  }
  size_t distance = end - start;
  char arr[distance + 1];
  memcpy(arr, start, distance);
  arr[distance] = '\0';
  Token* tok = newtok(type, arr);
  if (!tok) {
    return nullptr;
  }
  return tok;
}

int lx_tokenize(TokenList* dst, CharStream* in) {
  errno = 0;
  Token* tok = nullptr;
  do {
    tok = lx_token(in);
    tokenlist_append_tok(dst, tok);
  } while (tok != nullptr);
  if (errno != 0) {
      Token* iter = dst->head;
    while (iter != nullptr) {
      Token* tmp = iter;
      iter = iter->next;
      token_delete(tmp);
      dst->size--;
    }
  }

  return errno;
}

bool lx_end_of_token(int c, QuoteFlagE flag) {
  if (flag == UNQUOTED) {
    return isspace(c) || c == -1;
  } else {
    return c == -1;
  }
}
