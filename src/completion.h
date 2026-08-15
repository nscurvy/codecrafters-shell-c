//
// Created by nkinder on 8/14/26.
//


#pragma once

char* builtin_generator(const char* text, int state);

char* path_generator(const char* text, int state);

char* first_word_generator(const char* text, int state);

char** shell_completion_function(const char* text, int start, int end);