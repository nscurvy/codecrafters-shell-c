//
// Created by nkinder on 8/14/26.
//
//


#pragma once
#include "nullability.h"

#include <stdio.h>

#include <readline/readline.h>

ASSUME_NONNULL_BEGIN
typedef rl_compentry_func_t CompletionGenerator;


typedef struct CompletionRegistration {
    const char* command;
    const char* script_path;
} CompletionRegistration;

#define MAX_COMPLETIONS 64
extern CompletionRegistration completion_registry[MAX_COMPLETIONS];
extern int                    registry_count;

void
register_completion(const char* command, const char* script_path) GCC_NONNULL(1, 2);

void
unregister_completion(const char* command) GCC_NONNULL(1);

const char* NULLABLE
lookup_completion(const char* command) GCC_NONNULL(1);

const char* NULLABLE
find_current_word(const char* text) GCC_NONNULL(1);

const char* NULLABLE
get_previous_word(const char* current_word) GCC_NONNULL(1);

char* NULLABLE
get_command_word();

char* NULLABLE
external_completer_generator(const char* text, int state) GCC_NONNULL(1);

// TODO: DOCS
char* NULLABLE
builtin_generator(const char* text, int state) GCC_NONNULL(1);

// TODO: DOCS
char* NULLABLE
path_generator(const char* text, int state) GCC_NONNULL(1);

// TODO: DOCS
char* NULLABLE
first_word_generator(const char* text, int state) GCC_NONNULL(1);

// TODO: DOCS
char** NULLABLE
shell_completion_function(const char* text, int start, int end) GCC_NONNULL(1);

ASSUME_NONNULL_END
