//
// Created by nkinder on 8/11/26.
//

#pragma once

enum BuiltinE {
  BUILTIN_CD,
  BUILTIN_ECHO,
  BUILTIN_EXIT,
  BUILTIN_PWD,
  BUILTIN_TYPE,
  NUMBUILTINS
};

typedef int (*cmd_func)(const int,const char**);

typedef struct BuiltinCmd {
  const char* name;
  cmd_func builtin;
} BuiltinCmd;

// TODO: DOCS
extern const BuiltinCmd builtins[NUMBUILTINS];

// TODO: DOCS
int pstrcmp(const void* a, const void* b);


