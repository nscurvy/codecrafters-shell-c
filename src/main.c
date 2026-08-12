#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int repl();

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);


  return repl();
}

int repl() {
    char command[1024];
    bool running = true;
    while (running) {
      printf("$ ");
      setbuf(stdout, NULL);
      const char* res = fgets(command, sizeof(command), stdin);
      if (res && !ferror(stdin)) { // Check to make sure some error hasn't occured before
        // trusting that command has a sane value
        command[strcspn(command, "\n")] = '\0'; // Remove the newline.
        if (strncmp(command, "exit", 4) == 0) {
          exit(EXIT_SUCCESS);
        }
        printf("%s: command not found\n", command);
        setbuf(stdout, NULL);
      }
    }
    return 0;
}
