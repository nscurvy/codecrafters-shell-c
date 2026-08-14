//
// Created by nkinder on 8/11/26.
//

#pragma once

#define NUMBUILTINS 3

typedef int (*cmd_func)(int,char**);

typedef struct BuiltinCmd {
  const char* name;
  cmd_func builtin;
} BuiltinCmd;

extern const BuiltinCmd builtins[NUMBUILTINS];

int pstrcmp(const void* a, const void* b);
