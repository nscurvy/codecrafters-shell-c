//
// Created by nkinder on 9/14/26.
//

#pragma once
#include "nullability.h"
#include <stddef.h>
ASSUME_NONNULL_BEGIN


const char*
var_lookup(const char* name) GCC_NONNULL(1);

const char*
var_lookupn(const char* name, size_t count) GCC_NONNULL(1);

void
var_assign(const char* name, const char* value) GCC_NONNULL(1, 2);

void
var_export(const char* name, const char* value) GCC_NONNULL(1, 2);

bool
var_unassign(const char* name) GCC_NONNULL(1);

void
fork_exported();

void
var_init_from_environ();
ASSUME_NONNULL_END
