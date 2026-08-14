//
// Created by nkinder on 8/13/26.
//

#pragma once
#include "builtins.h"

char* find_command(char* dest, const char* command);

int execc(int argc, char** argv);

BuiltinCmd* find_builtin(const char* name);

int repl();
struct WordList;

void prepare_args(char** dest, struct WordList* words);

