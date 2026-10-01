#include "completion.h"
#include <readline/readline.h>

struct HashTable;

extern struct HashTable* variable_table;


int
repl();

int
main(int argc, char* argv[]) {
    // Flush after every printf
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    rl_attempted_completion_function = shell_completion_function;


    return repl();
}
