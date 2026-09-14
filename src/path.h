//
// Created by nkinder on 9/10/26.
//

#pragma once

#include "nullability.h"
#include "wordlist.h"
#include <stddef.h>

ASSUME_NONNULL_BEGIN

bool has_path_components(const char* path)
GCC_NONNULL(1);

WordList* NULLABLE
path_split(const char* path_value)
GCC_NONNULL(1);

const char*
path_getenv();

WordList*
path_get_dirs();

char* NULLABLE
path_find_command(const char* command)
GCC_NONNULL(1);

bool
path_is_executable_file(const char* path)
GCC_NONNULL(1);

char* NULLABLE
path_join(const char* dir, const char* name)
GCC_NONNULL(1,2);

ASSUME_NONNULL_END