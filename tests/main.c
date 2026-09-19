//
// Created by nkinder on 9/5/26.
//
#include "../tests/tests.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUGGER_MODE
// #UNDEF DEBUGGER_MODE

int
main() {
    int      number_failed;
    Suite*   lexer_s      = lexer_suite();
    SRunner* suite_runner = srunner_create(lexer_s);
    Suite*   parser_s     = parser_suite();
    Suite*   sb_s         = sb_suite();
    Suite*   jobs_s       = jobs_suite();
    Suite*   exec_s       = exec_suite();
    Suite*   path_s       = path_suite();
    Suite*   var_s        = var_suite();
    Suite*   expand_s     = expand_suite();

    srunner_add_suite(suite_runner, parser_s);
    srunner_add_suite(suite_runner, sb_s);
    srunner_add_suite(suite_runner, jobs_s);
    srunner_add_suite(suite_runner, exec_s);
    srunner_add_suite(suite_runner, path_s);
    srunner_add_suite(suite_runner, var_s);
    srunner_add_suite(suite_runner, expand_s);


#ifdef DEBUGGER_MODE
    srunner_set_fork_status(suite_runner, CK_NOFORK);
#endif


    srunner_run_all(suite_runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(suite_runner);
    srunner_free(suite_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
