//
// Created by nkinder on 8/11/26.
//

#pragma once
#include "nullability.h"

enum BuiltinE {
  BUILTIN_CD,
  BUILTIN_COMPLETE,
  BUILTIN_ECHO,
  BUILTIN_EXIT,
  BUILTIN_JOBS,
  BUILTIN_PWD,
  BUILTIN_TYPE,
  NUMBUILTINS
};

ASSUME_NONNULL_BEGIN

typedef int (*cmd_func)(const int,const char**);

typedef struct BuiltinCmd {
  const char* name;
  cmd_func builtin;
} BuiltinCmd;

// TODO: DOCS
extern const BuiltinCmd builtins[NUMBUILTINS];

// TODO: DOCS
int pstrcmp(const void* a, const void* b)
GCC_NONNULL(1,2);

ASSUME_NONNULL_END