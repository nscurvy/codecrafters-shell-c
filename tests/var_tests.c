//
// Created by nkinder on 9/19/26.
//
#include "hashtable.h"
#include "tests_common.h"
#include "unistd.h"
#include "vars.h"

#include <stdlib.h>

extern struct HashTable* variable_table;
struct HashTable**       vtp = &variable_table;

START_TEST(test_inititalization_from_environ) {
    setenv("TEST", "success", 0);
    var_init_from_environ();

    const char* result = var_lookup("TEST");
    streq(result, "success");
}
END_TEST


START_TEST(test_lookup_works_without_init) {
    if (variable_table) {
        ht_delete(variable_table);
        *vtp = nullptr;
    }
    const char* result = var_lookup("Nothing");

    assert(result == nullptr);
}
END_TEST

START_TEST(test_putting_works) {
    var_assign("test", "success");

    const char* result = var_lookup("test");
    streq(result, "success");
}
END_TEST

START_TEST(test_lookupn_for_partials) {
    var_assign("test", "success");

    const char* result = var_lookupn("testareallylongname", 4);
    streq(result, "success");
}
END_TEST

START_TEST(test_fork_exports_correctly) {
    var_assign("test", "failure");
    var_export("TEST", "success");
    fork_exported();

    const char* first  = var_lookup("test");
    const char* second = var_lookup("TEST");
    assert(first == nullptr);
    streq(second, "success");
}
END_TEST


Suite*
var_suite() {
    Suite* suite = suite_create("Variable Tests");

    TCase* variable_table_case = tcase_create("Test Variable Table");
    TCase* init_case           = tcase_create("Test initialization");

    tcase_add_test(init_case, test_inititalization_from_environ);
    tcase_add_test(init_case, test_lookup_works_without_init);
    tcase_add_test(variable_table_case, test_putting_works);
    tcase_add_test(variable_table_case, test_lookupn_for_partials);
    tcase_add_test(variable_table_case, test_fork_exports_correctly);


    suite_add_tcase(suite, variable_table_case);
    suite_add_tcase(suite, init_case);
    return suite;
}
