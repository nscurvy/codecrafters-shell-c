//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "builtins.h"
struct Command;

char* find_command(char* dest, const char* command);

int execc(struct Command* command);

//BuiltinCmd* find_builtin(const char* name);
BuiltinCmd* find_builtin(const char* name);

int repl();
struct WordList;

void prepare_args(char** dest, struct WordList* words);

int execute_command(struct Command* command);
