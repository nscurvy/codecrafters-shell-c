#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  char command[1024];
  printf("$ ");
  const char* res = fgets(command, sizeof(command), stdin);
  if (res && !ferror(stdin)) { // Check to make sure some error hasn't occured before
                               // trusting that command has a sane value
    command[strcspn(command, "\n")] = '\0'; // Remove the newline.
    printf("%s: command not found\n", command);
  }

  return 0;
}
