//
// Created by nkinder on 8/14/26.
//
//
#include <glib-2.0/glib.h>


#pragma once

typedef struct CustomCompletion {
    const char* name;

};

extern GHashTable* completion_table;

// TODO: DOCS
char* builtin_generator(const char* text, int state);

// TODO: DOCS
char* path_generator(const char* text, int state);

// TODO: DOCS
char* first_word_generator(const char* text, int state);

// TODO: DOCS
char** shell_completion_function(const char* text, int start, int end);

