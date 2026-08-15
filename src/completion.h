//
// Created by nkinder on 8/14/26.
//
//
#include <readline/readline.h>


#pragma once

typedef rl_compentry_func_t CompletionGenerator;


typedef struct CompletionRegistration {
    const char* command;
    const char* script_path;
} CompletionRegistration;

#define MAX_COMPLETIONS 64
extern CompletionRegistration completion_registry[MAX_COMPLETIONS];
extern int registry_count;

void register_completion(const char* command, const char* script_path);
const char* lookup_completion(const char* command);


char* get_command_word();

// TODO: DOCS
char* builtin_generator(const char* text, int state);

// TODO: DOCS
char* path_generator(const char* text, int state);

// TODO: DOCS
char* first_word_generator(const char* text, int state);

// TODO: DOCS
char** shell_completion_function(const char* text, int start, int end);

