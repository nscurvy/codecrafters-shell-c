//
// Created by nkinder on 8/14/26.
//

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "completion.h"
#include "builtins.h"


CompletionRegistration completion_registry[MAX_COMPLETIONS];
int registry_count = 0;



void
register_completion(const char* command, const char* script_path) {
    for (int i = 0; i < registry_count; ++ i) {
        if (strcmp(completion_registry[i].command, command) == 0) {
            free(completion_registry[i].script_path);
            completion_registry[i].script_path = strdup(script_path);
            return;
        }
    }
    if (registry_count >= MAX_COMPLETIONS) {
        return;
    }
    completion_registry[registry_count].command = strdup(command);
    completion_registry[registry_count].script_path = strdup(script_path);
    registry_count++;
}



const char*
lookup_completion(const char* command) {
    for (int i = 0; i < registry_count; ++ i) {
        if (strcmp(completion_registry[i].command, command) == 0) {
            return completion_registry[i].script_path;
        }
    }
    return nullptr;
}



char*
get_command_word() {
    if (!rl_line_buffer) {
        return nullptr;
    }
    const char* p = rl_line_buffer;
    while (*p == ' ') {
        ++p;
    }
    const char* end = p;
    while (*end && *end != ' ') {
        ++end;
    }
    if (end == p) {
        return nullptr;
    }

    return strndup(p, end - p);
}

char *
builtin_generator(const char *text, int state) {
    static int list_index, len;
    char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = (char*)builtins[list_index].name)) {
        list_index++;
        if (list_index == NUMBUILTINS) {
            return nullptr;
        }

        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return nullptr;
}

char *
path_generator(const char *text, int state) {
    static char *path_env = nullptr;
    static char* path_copy = nullptr;
    static char* dir_token = nullptr;
    static DIR* current_dir = nullptr;
    static int len = 0;

    if (!state) {
        if (current_dir) {
            closedir(current_dir);
            current_dir = nullptr;
        }
        free(path_copy);
        path_copy = nullptr;

        path_env = getenv("PATH");
        if (!path_env) {
            return nullptr;
        }

        path_copy = strdup(path_env);
        dir_token = strtok(path_copy, ":");

        if (dir_token) {
            current_dir = opendir(dir_token);
        }
        len = strlen(text);
    }

    while (dir_token != nullptr) {
        if (current_dir == nullptr) {
            dir_token = strtok(nullptr, ":");
            if (dir_token) {
                current_dir = opendir(dir_token);
            }
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(current_dir)) != nullptr) {
            if (strncmp(entry->d_name, text, len) == 0) {
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_token, entry->d_name);

                struct stat st;

                if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
                    return strdup(entry->d_name);
                }
            }
        }

        closedir(current_dir);
        current_dir = nullptr;
        dir_token = strtok(nullptr, ":");
        if (dir_token) {
            current_dir = opendir(dir_token);
        }
    }

    free(path_copy);
    path_copy = nullptr;
    return nullptr;

}

char *
first_word_generator(const char *text, int state) {
    static int phase = 0;
    if (!state) {
        phase = 0;
    }

    if (phase == 0) {
        char* match = builtin_generator(text, state);
        if (match) {
            return match;
        }
        phase = 1;
        state = 0;
    }
    return path_generator(text, state);
}

char **
shell_completion_function(const char *text, int start, int end) {
    if (rl_line_buffer != nullptr) {
        if (strncmp(rl_line_buffer, "./", 2) == 0 || strncmp(rl_line_buffer, "../", 3) == 0) {
            rl_attempted_completion_over = 0;
            return nullptr;
        }
    }
    if (start == 0) {
        rl_attempted_completion_over = 1;
        return rl_completion_matches(text, first_word_generator);
    }

    //char* cmd = get_command_word();
    //if (cmd) {
    //    CompletionGenerator* gen = lookup_completion(cmd);
    //    free(cmd);
    //    if (gen) {
    //        rl_attempted_completion_over = 1;
    //        return rl_completion_matches(text, gen);
    //    }
    //}

    rl_attempted_completion_over = 0;
    return nullptr;
}
