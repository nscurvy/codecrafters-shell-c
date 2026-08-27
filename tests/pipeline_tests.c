//
// Created by nkinder on 8/26/26.
//
#include <check.h>
#include <stdio.h>

#include "parser.h"

WordList* generate_one_pipe_wordlist() {
    WordList* wordlist = empty_wordlist();
    append_wordlist(wordlist, "echo");
    append_wordlist(wordlist, "hello");
    append_wordlist(wordlist, "world");
    append_wordlist(wordlist, "|");
    append_wordlist(wordlist, "ls");
    append_wordlist(wordlist, ".");
    return wordlist;
}

WordList* generate_two_pipe_wordlist() {
    WordList* wordlist = empty_wordlist();
    append_wordlist(wordlist, "echo");
    append_wordlist(wordlist, "hello");
    append_wordlist(wordlist, "world");
    append_wordlist(wordlist, "|");
    append_wordlist(wordlist, "ls");
    append_wordlist(wordlist, ".");
    append_wordlist(wordlist, "|");
    append_wordlist(wordlist, "echo");
    return wordlist;
}

START_TEST(test_split_wordlist_on_one_pipe) {
    WordList* test_list = generate_one_pipe_wordlist();
    WordList* dest = empty_wordlist();
    bool result = split_on_pipes(dest, test_list);

    ck_assert(result);
    ck_assert(dest->size == 2);
    ck_assert(test_list->size == 3);
    ck_assert_str_eq(dest->head->value, "ls");
    ck_assert_str_eq(dest->head->next->value, ".");
    ck_assert_ptr_eq(dest->head->next->next, nullptr);
    ck_assert_str_eq(test_list->head->value, "echo");
    ck_assert_str_eq(test_list->head->next->value, "hello");
    ck_assert_str_eq(test_list->head->next->next->value, "world");
    ck_assert_ptr_eq(test_list->head->next->next->next, nullptr);

    cleanup_wordlist(dest);
    cleanup_wordlist(test_list);
}
END_TEST

START_TEST(test_split_on_two_pipes) {
    WordList* lista = generate_two_pipe_wordlist();
    WordList* listb = empty_wordlist();
    WordList* listc = empty_wordlist();

    bool result1 = split_on_pipes(listb, lista);
    bool result2 = split_on_pipes(listc, listb);
    ck_assert(result1);
    ck_assert(result2);
    ck_assert(listc->size == 1);
    ck_assert_str_eq(listc->head->value, "echo");
    ck_assert_str_eq(listb->head->value, "ls");
    ck_assert_str_eq(lista->head->value, "echo");
    cleanup_wordlist(lista);
    cleanup_wordlist(listb);
    cleanup_wordlist(listc);
}
END_TEST

START_TEST(test_build_pipeline_from_commands) {
    WordList* wordlist = generate_one_pipe_wordlist();
    Pipeline* pipeline = build_pipeline(wordlist);

    ck_assert(pipeline->ncmds == 2);
    ck_assert_str_eq(pipeline->cmds[0]->argv[0], "echo");
    ck_assert_str_eq(pipeline->cmds[1]->argv[0], "ls");

    cleanup_wordlist(wordlist);
    cleanup_pipeline(pipeline);
}

START_TEST(test_build_two_pipe_pipeline_from_commands) {
    WordList* wordlist = generate_two_pipe_wordlist();
    Pipeline* pipeline = build_pipeline(wordlist);

    ck_assert(pipeline->ncmds == 3);
    ck_assert_str_eq(pipeline->cmds[0]->argv[0], "echo");
    ck_assert_str_eq(pipeline->cmds[1]->argv[0], "ls");
    ck_assert_str_eq(pipeline->cmds[2]->argv[0], "echo");

    cleanup_wordlist(wordlist);
    cleanup_pipeline(pipeline);
}

START_TEST(test_build_pipeline_from_no_pipes) {
    WordList* wordlist = empty_wordlist();
    append_wordlist(wordlist, "echo");
    append_wordlist(wordlist, "hello");
    append_wordlist(wordlist, "world");
    Pipeline* pipeline = build_pipeline(wordlist);

    ck_assert(pipeline->ncmds == 1);
    ck_assert_str_eq(pipeline->cmds[0]->argv[0], "echo");

    cleanup_wordlist(wordlist);
    cleanup_pipeline(pipeline);
}

Suite* pipeline_suite(void) {
    Suite* s;
    TCase* tc_core;

    s = suite_create("Pipeline");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_split_wordlist_on_one_pipe);
    tcase_add_test(tc_core, test_split_on_two_pipes);
    tcase_add_test(tc_core, test_build_pipeline_from_commands);
    tcase_add_test(tc_core, test_build_two_pipe_pipeline_from_commands);
    tcase_add_test(tc_core, test_build_pipeline_from_no_pipes);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = pipeline_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}