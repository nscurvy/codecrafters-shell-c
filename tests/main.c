//
// Created by nkinder on 9/5/26.
//
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include "../tests/tests.h"

#define DEBUGGER_MODE
//#UNDEF DEBUGGER_MODE

int main() {
  int number_failed;
  Suite* s;
  SRunner* sr;

  s = lexer_suite();
  sr = srunner_create(s);

#ifdef DEBUGGER_MODE
  srunner_set_fork_status(sr, CK_NOFORK);
#endif

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
