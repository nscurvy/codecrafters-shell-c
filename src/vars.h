//
// Created by nkinder on 9/14/26.
//

#pragma once
#include <stddef.h>


const char*
var_lookup(const char* name);

const char*
var_lookupn(const char* name, size_t count);

void
var_assign(const char* name, const char* value);

void
var_export(const char* name, const char* value);

bool
var_unassign(const char* name);

void
fork_exported();

void
var_init_from_environ();
