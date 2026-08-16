//
// Created by nkinder on 8/14/26.
//

#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "completion.h"

#include <iso646.h>
#include <sys/wait.h>

#include "builtins.h"
#include "parser.h"


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

void unregister_completion(const char* command) {
    if (lookup_completion(command)) {
        int i = 0;
        while (strcmp(completion_registry[i].command, command ) != 0) {
            ++i;
        }
        CompletionRegistration tmp = completion_registry[i];

        for (int j = i; j < registry_count - 1; ++ j) {
            completion_registry[j] = completion_registry[j + 1];
        }
        --registry_count;
        free(tmp.command);
        free(tmp.script_path);
    }
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

const char* skipws(const char* s) {
    while (*s == ' ') {
        ++s;
    }
    return s;
}

const char*
find_current_word(const char* text) {
    if (!rl_line_buffer) {
        return nullptr;
    }
    size_t textlen = strlen(text);
    char* p = rl_line_buffer;
    p = skipws(p);
    while (strncmp(p, text, textlen) != 0) {
        while (*p && *p != ' ') {
            ++p;
        }
        p = skipws(p);
    }

    return p;

}


const char* goto_next_word(const char* s) {
    if (*s == '\0') {
        return nullptr;
    }
    if (*s == ' ') {
        s = skipws(s);
    }
    const char* end = s;
    while (*end && *end != ' ') {
        ++end;
    }
    if (end == s) {
        return nullptr;
    }
    s = end;
    if (*end && *end == ' ') {
        end = skipws(end);
    }
    if (s == end) {
        return nullptr;
    }
    return end;
}

const char*
    get_previous_word(const char* current_word) {
    if (!rl_line_buffer) {
        return nullptr;
    }
    const char* p = rl_line_buffer;

    p = skipws(p);

    if (p == current_word) {
        return nullptr;
    }

    int prev_start = 0;
    int prev_end = 0;
    while ((p - rl_line_buffer) != (current_word - rl_line_buffer)) {
        prev_start = p - rl_line_buffer;
        prev_end = prev_start;

        const char* end = p;
        while (*end && *end != ' ') {
            ++end;
        }
        prev_end = end - rl_line_buffer;
        p = skipws(end);
    }
    return strndup(rl_line_buffer + prev_start, prev_end - prev_start);


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

    char* cmd = get_command_word();
    if (cmd) {
        const char* script_path = lookup_completion(cmd);
        free(cmd);
        if (script_path) {
            rl_attempted_completion_over = 1;
            return rl_completion_matches(text, external_completer_generator);
        }
    }


    rl_attempted_completion_over = 0;
    return nullptr;
}
char*
    external_completer_generator(const char* text, int state) {
    static char* cached_result = nullptr;
    static int cmp_idx = 0;
    static size_t cmp_len = 0;
    static char** completions = nullptr;

    if (!state) {
        free(cached_result);
        cached_result = nullptr;
    }



    if (!state) {
        char prev_wordbuf[1024] = {0};
        char cmd_wordbuf[1024] = {0};
        char* cmd = get_command_word();
        const char* script_path = cmd ? lookup_completion(cmd) : nullptr;
        if (cmd) {
            memmove(cmd_wordbuf, cmd, strlen(cmd));
            free(cmd);
            cmd = cmd_wordbuf;
        }
        const char* current_word = find_current_word(text);
        const char* prev_word = get_previous_word(current_word);
        if (prev_word == nullptr) {
            prev_word = "";
        } else {
            memmove(prev_wordbuf, prev_word, strlen(prev_word));
            free(prev_word);
            prev_word = prev_wordbuf;
        }
        if (!script_path) {
            if (completions) {
                free(completions);
                completions = nullptr;
                cmp_len = 0;
                cmp_idx = 0;
            }
            return nullptr;
        }

        int pipefd[2];
        if (pipe(pipefd) < 0) {

            if (completions) {
                free(completions);
                completions = nullptr;
                cmp_len = 0;
                cmp_idx = 0;
            }
            return nullptr;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(pipefd[0]);
            close(pipefd[1]);

            if (completions) {
                free(completions);
                completions = nullptr;
                cmp_len = 0;
                cmp_idx = 0;
            }
            return nullptr;
        }
        if (pid == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            char comp_line[1024] = {0};
            char comp_point[1024] = {0};
            char* const envp[3] = { comp_line, comp_point, nullptr};
            snprintf(comp_line, sizeof(comp_line), "COMP_LINE=%s", rl_line_buffer);
            snprintf(comp_point, sizeof(comp_point), "COMP_POINT=%lu", strlen(rl_line_buffer));
            execle(script_path, script_path, cmd, current_word, prev_word, nullptr, envp);
            _exit(127);
        }

        close(pipefd[1]);
        FILE* f = fdopen(pipefd[0], "r");
        char line[1024] = {0};
        WordList* words = empty_wordlist();
        while (fgets(line, sizeof(line), f) != nullptr) {
            line[strcspn(line, "\n")] = '\0';
            append_wordlist(words, line);
            //cached_result = strdup(line);
        }
        fclose(f);

        completions = malloc(sizeof(char*) * words->size);
        WordNode* iter = words->head;
        for (int i = 0; i < words->size; ++ i) {
            completions[i] = strdup(iter->value);
            iter = iter->next;
        }
        cmp_len = words->size;
        cleanup_wordlist(words);
        int status;
        waitpid(pid, &status, 0);
    }
    if (completions) {
        if (cmp_idx >= cmp_len) {
            free(completions);
            completions = nullptr;
            cmp_len = 0;
            cmp_idx = 0;
            return nullptr;
        }
        char* ret = completions[cmp_idx++];
        return ret;
    }
    return nullptr;
}
