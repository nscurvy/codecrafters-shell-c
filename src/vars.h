//
// Created by nkinder on 9/14/26.
//

#pragma once

typedef struct Variable {
    const char* name;
    const char* value;
    bool        exported;
} Variable;


Variable*
var_new(const char* name, const char* value, bool exported);

void
var_delete(Variable* var);

const char*
var_lookup(const char* name);

const char*
var_assign(const char* name, const char* value);

const char*
var_export(const char* name, const char* value);
