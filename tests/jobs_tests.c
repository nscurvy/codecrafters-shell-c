//
// Created by nkinder on 9/12/26.
//
#include <check.h>

#include "jobs.h"
#include "parsetypes.h"
#include "tests_common.h"
#include "stringbuilder.h"

static Redirect NO_REDIRS[1]; /* never dereferenced when nredirs == 0 */

/* ------------------------------------------------------------------ */
/* Fixture helpers                                                      */
/* ------------------------------------------------------------------ */

static Command* simple_command(const char** argv) {
    Command* cmd = command_new(argv, NULL, 0, NO_REDIRS);
    assert(cmd != NULL);
    return cmd;
}

static Command* command_with_assignment(const char* assignment, const char** argv) {
    Assignment* a = ass_new(assignment);
    assert(a != NULL);
    Command* cmd = command_new(argv, a, 0, NO_REDIRS);
    assert(cmd != NULL);
    return cmd;
}

static Command* command_with_redirs(const char** argv, Redirect* redirs, size_t nredirs) {
    Command* cmd = command_new(argv, NULL, nredirs, redirs);
    assert(cmd != NULL);
    return cmd;
}

static Pipeline* build_pipeline(Command** cmds, size_t n) {
    Pipeline* pl = pipeline_new_empty();
    assert(pl != NULL);
    PipelineElement* tail = NULL;
    for (size_t i = 0; i < n; ++i) {
        PipelineElement* elem = ple_new(cmds[i]);
        assert(elem != NULL);
        if (pl->head == NULL) {
            pl->head = elem;
        } else {
            tail->next = elem;
        }
        tail = elem;
    }
    pl->size = n;
    return pl;
}

static AndOr* build_andor(Pipeline** pipelines, AndOrOp* ops, size_t n) {
    AndOr* ao = ao_new_empty();
    assert(ao != NULL);
    AndOrElement* tail = NULL;
    for (size_t i = 0; i < n; ++i) {
        AndOrElement* elem = aoe_new(pipelines[i], ops[i]);
        assert(elem != NULL);
        if (ao->head == NULL) {
            ao->head = elem;
        } else {
            tail->next = elem;
        }
        tail = elem;
    }
    ao->count = n;
    return ao;
}

/* ------------------------------------------------------------------ */
/* join_redirect                                                       */
/* ------------------------------------------------------------------ */

START_TEST(test_join_redirect_out_default_fd_omitted) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 1, .mode = REDIR_OUT, .target = "out.txt" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, "> out.txt");
  sb_delete(sb);
}
END_TEST

START_TEST(test_join_redirect_out_explicit_fd_shown) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 2, .mode = REDIR_OUT, .target = "err.txt" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, "2> err.txt");
  sb_delete(sb);
}
END_TEST

START_TEST(test_join_redirect_append_default_fd_omitted) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 1, .mode = REDIR_APPEND, .target = "out.txt" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, ">> out.txt");
  sb_delete(sb);
}
END_TEST

START_TEST(test_join_redirect_in_default_fd_omitted) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 0, .mode = REDIR_IN, .target = "in.txt" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, "< in.txt");
  sb_delete(sb);
}
END_TEST

START_TEST(test_join_redirect_in_explicit_fd_shown) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 3, .mode = REDIR_IN, .target = "in.txt" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, "3< in.txt");
  sb_delete(sb);
}
END_TEST

START_TEST(test_join_redirect_heredoc) {
  StringBuilder* sb = sb_new();
  Redirect r = { .fd = 0, .mode = REDIR_HEREDOC, .target = "EOF" };
  join_redirect(&r, sb);
  ck_assert_str_eq(sb->str, "<< EOF");
  sb_delete(sb);
}
END_TEST

/* ------------------------------------------------------------------ */
/* join_command                                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_join_command_simple) {
  const char* argv[] = { "echo", "hi", NULL };
  Command* cmd = simple_command(argv);

  StringBuilder* sb = sb_new();
  join_command(cmd, sb);
  ck_assert_str_eq(sb->str, "echo hi");

  sb_delete(sb);
  command_delete(cmd);
}
END_TEST

START_TEST(test_join_command_with_leading_assignment) {
  const char* argv[] = { "echo", "hi", NULL };
  Command* cmd = command_with_assignment("FOO=bar", argv);

  StringBuilder* sb = sb_new();
  join_command(cmd, sb);
  ck_assert_str_eq(sb->str, "FOO=bar echo hi");

  sb_delete(sb);
  command_delete(cmd);
}
END_TEST

START_TEST(test_join_command_with_single_redirect) {
  const char* argv[] = { "echo", "hi", NULL };
  Redirect redirs[] = { { .fd = 1, .mode = REDIR_OUT, .target = "out.txt" } };
  Command* cmd = command_with_redirs(argv, redirs, 1);

  StringBuilder* sb = sb_new();
  join_command(cmd, sb);
  ck_assert_str_eq(sb->str, "echo hi > out.txt");

  sb_delete(sb);
  command_delete(cmd);
}
END_TEST

START_TEST(test_join_command_with_explicit_fd_redirect) {
  const char* argv[] = { "cmd", NULL };
  Redirect redirs[] = { { .fd = 2, .mode = REDIR_OUT, .target = "err.txt" } };
  Command* cmd = command_with_redirs(argv, redirs, 1);

  StringBuilder* sb = sb_new();
  join_command(cmd, sb);
  ck_assert_str_eq(sb->str, "cmd 2> err.txt");

  sb_delete(sb);
  command_delete(cmd);
}
END_TEST

START_TEST(test_join_command_with_multiple_redirects) {
  const char* argv[] = { "cmd", NULL };
  Redirect redirs[] = {
      { .fd = 0, .mode = REDIR_IN, .target = "in.txt" },
      { .fd = 2, .mode = REDIR_OUT, .target = "err.txt" },
  };
  Command* cmd = command_with_redirs(argv, redirs, 2);

  StringBuilder* sb = sb_new();
  join_command(cmd, sb);
  ck_assert_str_eq(sb->str, "cmd < in.txt 2> err.txt");

  sb_delete(sb);
  command_delete(cmd);
}
END_TEST

/* ------------------------------------------------------------------ */
/* join_pipeline                                                       */
/* ------------------------------------------------------------------ */

START_TEST(test_join_pipeline_single_command) {
  const char* argv[] = { "echo", "hi", NULL };
  Command* cmd = simple_command(argv);
  Command* cmds[] = { cmd };
  Pipeline* pl = build_pipeline(cmds, 1);

  StringBuilder* sb = sb_new();
  join_pipeline(pl, sb);
  ck_assert_str_eq(sb->str, "echo hi");

  sb_delete(sb);
  pipeline_delete(pl);
}
END_TEST

START_TEST(test_join_pipeline_two_commands) {
  const char* argv1[] = { "echo", "hi", NULL };
  const char* argv2[] = { "wc", "-l", NULL };
  Command* cmds[] = { simple_command(argv1), simple_command(argv2) };
  Pipeline* pl = build_pipeline(cmds, 2);

  StringBuilder* sb = sb_new();
  join_pipeline(pl, sb);
  ck_assert_str_eq(sb->str, "echo hi | wc -l");

  sb_delete(sb);
  pipeline_delete(pl);
}
END_TEST

START_TEST(test_join_pipeline_three_commands) {
  const char* argv1[] = { "cat", "file.txt", NULL };
  const char* argv2[] = { "grep", "foo", NULL };
  const char* argv3[] = { "wc", "-l", NULL };
  Command* cmds[] = { simple_command(argv1), simple_command(argv2), simple_command(argv3) };
  Pipeline* pl = build_pipeline(cmds, 3);

  StringBuilder* sb = sb_new();
  join_pipeline(pl, sb);
  ck_assert_str_eq(sb->str, "cat file.txt | grep foo | wc -l");

  sb_delete(sb);
  pipeline_delete(pl);
}
END_TEST

/* ------------------------------------------------------------------ */
/* join_andor                                                          */
/* ------------------------------------------------------------------ */

START_TEST(test_join_andor_single_pipeline) {
  const char* argv[] = { "echo", "hi", NULL };
  Command* cmd = simple_command(argv);
  Command* cmds[] = { cmd };
  Pipeline* pl = build_pipeline(cmds, 1);
  Pipeline* pipelines[] = { pl };
  AndOrOp ops[] = { AND_NONE };
  AndOr* ao = build_andor(pipelines, ops, 1);

  const char* result = join_andor(ao);
  ck_assert_str_eq(result, "echo hi");

  free((void*)result);
  ao_delete(ao);
}
END_TEST

START_TEST(test_join_andor_and_operator) {
  const char* argv1[] = { "true", NULL };
  const char* argv2[] = { "false", NULL };
  Pipeline* pl1 = build_pipeline((Command*[]){ simple_command(argv1) }, 1);
  Pipeline* pl2 = build_pipeline((Command*[]){ simple_command(argv2) }, 1);
  Pipeline* pipelines[] = { pl1, pl2 };
  AndOrOp ops[] = { AND_NONE, AND_AND };
  AndOr* ao = build_andor(pipelines, ops, 2);

  const char* result = join_andor(ao);
  ck_assert_str_eq(result, "true && false");

  free((void*)result);
  ao_delete(ao);
}
END_TEST

START_TEST(test_join_andor_mixed_and_or) {
  const char* argv1[] = { "true", NULL };
  const char* argv2[] = { "false", NULL };
  const char* argv3[] = { "true", NULL };
  Pipeline* pl1 = build_pipeline((Command*[]){ simple_command(argv1) }, 1);
  Pipeline* pl2 = build_pipeline((Command*[]){ simple_command(argv2) }, 1);
  Pipeline* pl3 = build_pipeline((Command*[]){ simple_command(argv3) }, 1);
  Pipeline* pipelines[] = { pl1, pl2, pl3 };
  AndOrOp ops[] = { AND_NONE, AND_AND, AND_OR };
  AndOr* ao = build_andor(pipelines, ops, 3);

  const char* result = join_andor(ao);
  ck_assert_str_eq(result, "true && false || true");

  free((void*)result);
  ao_delete(ao);
}
END_TEST

START_TEST(test_join_andor_pipeline_inside_chain) {
  /* echo hi | wc -l && true */
  const char* argv1[] = { "echo", "hi", NULL };
  const char* argv2[] = { "wc", "-l", NULL };
  const char* argv3[] = { "true", NULL };
  Command* pipe_cmds[] = { simple_command(argv1), simple_command(argv2) };
  Pipeline* pl1 = build_pipeline(pipe_cmds, 2);
  Pipeline* pl2 = build_pipeline((Command*[]){ simple_command(argv3) }, 1);
  Pipeline* pipelines[] = { pl1, pl2 };
  AndOrOp ops[] = { AND_NONE, AND_AND };
  AndOr* ao = build_andor(pipelines, ops, 2);

  const char* result = join_andor(ao);
  ck_assert_str_eq(result, "echo hi | wc -l && true");

  free((void*)result);
  ao_delete(ao);
}
END_TEST




Suite* jobs_suite() {
  Suite* suite = suite_create("Jobs");
  TCase* join_case = tcase_create("Test Joins");
  TCase* redirect_case = tcase_create("Test Join Redirection");
  TCase* command_case = tcase_create("Test Join Command");
  TCase* pipeline_case = tcase_create("Test Join Pipeline");
  TCase* andor_case = tcase_create("Test Join AndOr");


  tcase_add_test(redirect_case, test_join_redirect_out_default_fd_omitted);
  tcase_add_test(redirect_case, test_join_redirect_out_explicit_fd_shown);
  tcase_add_test(redirect_case, test_join_redirect_append_default_fd_omitted);
  tcase_add_test(redirect_case, test_join_redirect_in_default_fd_omitted);
  tcase_add_test(redirect_case, test_join_redirect_in_explicit_fd_shown);
  tcase_add_test(redirect_case, test_join_redirect_heredoc);
  tcase_add_test(command_case, test_join_command_simple);
  tcase_add_test(command_case, test_join_command_with_leading_assignment);
  tcase_add_test(command_case, test_join_command_with_single_redirect);
  tcase_add_test(command_case, test_join_command_with_explicit_fd_redirect);
  tcase_add_test(command_case, test_join_command_with_multiple_redirects);
  tcase_add_test(pipeline_case, test_join_pipeline_single_command);
  tcase_add_test(pipeline_case, test_join_pipeline_two_commands);
  tcase_add_test(pipeline_case, test_join_pipeline_three_commands);
  tcase_add_test(andor_case, test_join_andor_single_pipeline);
  tcase_add_test(andor_case, test_join_andor_and_operator);
  tcase_add_test(andor_case, test_join_andor_mixed_and_or);
  tcase_add_test(andor_case, test_join_andor_pipeline_inside_chain);

  suite_add_tcase(suite,redirect_case);
  suite_add_tcase(suite, command_case);
  suite_add_tcase(suite, pipeline_case);
  suite_add_tcase(suite, andor_case);

  return suite;

}