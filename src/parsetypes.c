//
// Created by nkinder on 9/10/26.
//

#include "parsetypes.h"
#include <stdlib.h>
#include <string.h>


Pipeline*
pipeline_new_empty() {
  Pipeline* pipeline = malloc(sizeof(Pipeline));
  if (!pipeline) {
    return nullptr;
  }
  pipeline->head = nullptr;
  pipeline->size = 0;
  return pipeline;
}

void
pipeline_delete(Pipeline* pipeline) {
  PipelineElement* iter = pipeline->head;
  while (iter != nullptr) {
    PipelineElement* tmp = iter;
    iter = iter->next;
    ple_delete(tmp);
  }
  free(pipeline);
}

PipelineElement*
  ple_new(Command* command) {
  PipelineElement* result = malloc(sizeof(PipelineElement));
  if (!result) {
    return nullptr;
  }

  result->command = command;
  result->next = nullptr;
  return result;
}

void ple_delete(PipelineElement* element) {
  command_delete(element->command);
  free(element);
}
void
command_delete(Command* command) {
  char** iter = command->argv;
  while (*iter != nullptr) {
    free(*iter++);
  }
  ass_delete_all(command->assignment_list);
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
argvlen(const char** argv) {
  size_t len  = 0;
  const char** iter = argv;
  while (*iter != nullptr) {
    len++;
    iter++;
  }
  return ++len;
}

Command*
command_new(const char** argv, Assignment* assignments, size_t nredirs, Redirect redirs[]) {
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

  command->argv    = args;
  command->nredirs = nredirs;
  command->assignment_list = assignments;
  memmove(command->redirs, redirs, sizeof(Redirect) * nredirs);


  return command;
}
Assignment*
ass_new(const char* value) {
  const char* cpy = strdup(value);
  if (!cpy) {
    return nullptr;
  }
  Assignment* ass = malloc(sizeof(Assignment));
  if (!ass) {
    free(cpy);
    return nullptr;
  }

  ass->value = cpy;
  ass->next = nullptr;
  return ass;
}

void
ass_delete(Assignment* assignment) {
  free(assignment->value);
  free(assignment);
}

void
ass_append(Assignment* head, Assignment* node) {
  Assignment* iter = head;
  while (iter->next != nullptr) {
    iter = iter->next;
  }
  iter->next = node;
}

Assignment*
ass_append_str(Assignment* head, const char* value) {
  Assignment* newnode = ass_new(value);

  if (!newnode) {
    return nullptr;
  }

  ass_append(head, newnode);
  return newnode;
}

void
ass_delete_all(Assignment* head) {
  Assignment* iter = head;
  Assignment* prev = nullptr;
  while (iter != nullptr) {
    Assignment* tmp = iter;
    prev = iter;
    iter = iter->next;
    ass_delete(tmp);
  }
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
  free(tokens);
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

size_t
ts_remaining(TokenStream* stream) {
  return stream->len - stream->pos;
}

List*
list_new_empty() {
  List* result = malloc(sizeof(List));
  if (!result) {
    return nullptr;
  }
  result->count = 0;
  result->head = nullptr;
  return result;
}

void
list_delete(List* list) {
  ListElement* iter = list->head;
  while (iter != nullptr) {
    ListElement* tmp = iter;
    iter = iter->next;
    le_delete(tmp);
  }
  free(list);
}

ListElement* le_new(AndOr* ao, ListSep op) {
  ListElement* result = malloc(sizeof(ListElement));
  if (!result) {
    return nullptr;
  }
  result->and_or = ao;
  result->next = nullptr;
  result->sep = op;
  return result;
}

void le_delete(ListElement* element) {
  ao_delete(element->and_or);
  free(element);
}



