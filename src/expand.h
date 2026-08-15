//
// Created by nkinder on 8/13/26.
//

#pragma once
/**
 * Perform expansion on a token word.
 * Allocates space for the expanded string.
 * @param word Null terminated string
 * @return a new heap allocated string with expansions performed.
 */
char * exptok(const char* word);

// TODO: DOCS
char* expand_home(char* buf, const char* path);
