//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "builtins.h"
struct Command;

// TODO: DOCS
char* find_command(char* dest, const char* command);

// TODO: DOCS
int execc(struct Command* command);

//BuiltinCmd* find_builtin(const char* name);
// TODO: DOCS
BuiltinCmd* find_builtin(const char* name);

// TODO: DOCS
int repl();
struct WordList;

/**
 * TODO: Documentation
 * @brief
 * @param dest
 * @param words
 */
void prepare_args(char** dest, struct WordList* words);

/**
 * TODO: this
 * @brief
 * @param command
 * @return
 */
int execute_command(struct Command* command);
