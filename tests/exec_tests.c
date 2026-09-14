//
// Created by nkinder on 9/13/26.
//

#include <check.h>
#include "exec.h"
#include "parsetypes.h"
#include "tests_common.h"

static Redirect NO_REDIRS_EXEC[1]; /* never dereferenced when nredirs == 0 */

bool should_run_next(int prev_status, AndOrOp op) {
  if (op == AND_AND) return prev_status == 0;
  if (op == AND_OR)  return prev_status != 0;
  return true; // shouldn't happen for op on a non-first element
}

START_TEST(test_should_run_next_and_short_circuits_on_failure) {
  assert(!should_run_next(1, AND_AND));
}
END_TEST

START_TEST(test_should_run_next_or_short_circuits_on_success) {
  assert(!should_run_next(0, AND_OR));
}
END_TEST

START_TEST(test_exec_command_returns_success_status) {
  const char* argv[] = { "true", NULL };
  Redirect dummy[1];
  Command* cmd = command_new(argv, nullptr, 0, dummy);
  int status = execute_command(cmd);
  inteq(status, 0);
  command_delete(cmd);
}
END_TEST

START_TEST(test_exec_command_returns_failure_status) {
  const char* argv[] = { "false", NULL };
  Redirect dummy[1];
  Assignment* ass = nullptr;
  Command* cmd = command_new(argv,ass, 0, dummy);
  int status = execute_command(cmd);
  ck_assert_int_ne(status, 0);
  command_delete(cmd);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Fixture helpers                                                      */
/* ------------------------------------------------------------------ */

static char* exec_temp_file(void) {
    char* path = strdup("/tmp/exec_test_XXXXXX");
    int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);
    return path;
}

static char* read_whole_file(const char* path) {
    FILE* f = fopen(path, "r");
    assert(f != NULL);
    char* buf = calloc(1, 4096);
    size_t n = fread(buf, 1, 4095, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static Command* build_command(const char** argv, Assignment* assignments, size_t nredirs, Redirect* redirs) {
    Command* cmd = command_new(argv, assignments, nredirs, redirs ? redirs : NO_REDIRS_EXEC);
    assert(cmd != NULL);
    return cmd;
}

/* ------------------------------------------------------------------ */
/* Exit status decoding                                                 */
/* ------------------------------------------------------------------ */

START_TEST(test_execute_command_success_status) {
  const char* argv[] = { "true", NULL };
  Command* cmd = build_command(argv, NULL, 0, NULL);
  inteq(execute_command(cmd), 0);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_failure_status) {
  const char* argv[] = { "false", NULL };
  Command* cmd = build_command(argv, NULL, 0, NULL);
  inteq(execute_command(cmd), 1);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_arbitrary_exit_code) {
  const char* argv[] = { "sh", "-c", "exit 42", NULL };
  Command* cmd = build_command(argv, NULL, 0, NULL);
  inteq(execute_command(cmd), 42);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_signal_terminated_status) {
  /* sh -c 'kill -9 $$' -- child kills itself with SIGKILL. Expect the
   * shell convention of 128 + signal number (128 + 9 = 137). */
  const char* argv[] = { "sh", "-c", "kill -9 $$", NULL };
  Command* cmd = build_command(argv, NULL, 0, NULL);
  inteq(execute_command(cmd), 137);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_not_found_returns_127) {
  const char* argv[] = { "this_command_should_not_exist_anywhere_12345", NULL };
  Command* cmd = build_command(argv, NULL, 0, NULL);
  inteq(execute_command(cmd), 127);
  command_delete(cmd);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Redirections                                                         */
/* ------------------------------------------------------------------ */

START_TEST(test_execute_command_redirect_out_writes_file) {
  char* out_path = exec_temp_file();
  const char* argv[] = { "echo", "hello", NULL };
  Redirect redirs[] = { { .fd = 1, .mode = REDIR_OUT, .target = out_path } };
  Command* cmd = build_command(argv, NULL, 1, redirs);

  inteq(execute_command(cmd), 0);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "hello\n");

  free(content);
  unlink(out_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_redirect_out_truncates_existing_content) {
  char* out_path = exec_temp_file();
  FILE* f = fopen(out_path, "w");
  fputs("old content that should be gone\n", f);
  fclose(f);

  const char* argv[] = { "echo", "new" , NULL };
  Redirect redirs[] = { { .fd = 1, .mode = REDIR_OUT, .target = out_path } };
  Command* cmd = build_command(argv, NULL, 1, redirs);

  execute_command(cmd);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "new\n");

  free(content);
  unlink(out_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_redirect_append_preserves_existing_content) {
  char* out_path = exec_temp_file();
  FILE* f = fopen(out_path, "w");
  fputs("first\n", f);
  fclose(f);

  const char* argv[] = { "echo", "second", NULL };
  Redirect redirs[] = { { .fd = 1, .mode = REDIR_APPEND, .target = out_path } };
  Command* cmd = build_command(argv, NULL, 1, redirs);

  execute_command(cmd);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "first\nsecond\n");

  free(content);
  unlink(out_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_redirect_in_reads_file) {
  char* in_path = exec_temp_file();
  FILE* fin = fopen(in_path, "w");
  fputs("line one\nline two\n", fin);
  fclose(fin);

  char* out_path = exec_temp_file();
  const char* argv[] = { "cat", NULL };
  Redirect redirs[] = {
      { .fd = 0, .mode = REDIR_IN, .target = in_path },
      { .fd = 1, .mode = REDIR_OUT, .target = out_path },
  };
  Command* cmd = build_command(argv, NULL, 2, redirs);

  inteq(execute_command(cmd), 0);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "line one\nline two\n");

  free(content);
  unlink(in_path);
  unlink(out_path);
  free(in_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_stderr_redirect) {
  char* out_path = exec_temp_file();
  const char* argv[] = { "sh", "-c", "echo err_text 1>&2", NULL };
  Redirect redirs[] = { { .fd = 2, .mode = REDIR_OUT, .target = out_path } };
  Command* cmd = build_command(argv, NULL, 1, redirs);

  execute_command(cmd);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "err_text\n");

  free(content);
  unlink(out_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Unsupported / malformed redirections                                 */
/* ------------------------------------------------------------------ */

START_TEST(test_execute_command_heredoc_redirect_currently_unsupported) {
  /* Per current implementation, REDIR_HEREDOC always _exit(1)s. Update
   * this test once heredocs are actually implemented. */
  const char* argv[] = { "cat", NULL };
  Redirect redirs[] = { { .fd = 0, .mode = REDIR_HEREDOC, .target = "EOF" } };
  Command* cmd = build_command(argv, NULL, 1, redirs);
  inteq(execute_command(cmd), 1);
  command_delete(cmd);
}
END_TEST

START_TEST(test_execute_command_input_redirect_bad_fd) {
  /* REDIR_IN with fd != 0 isn't supported and should _exit(1), not crash
   * or silently continue. */
  const char* argv[] = { "cat", NULL };
  Redirect redirs[] = { { .fd = 5, .mode = REDIR_IN, .target = "/dev/null" } };
  Command* cmd = build_command(argv, NULL, 1, redirs);
  inteq(execute_command(cmd), 1);
  command_delete(cmd);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Assignments -- KNOWN GAP, not yet implemented in execute_command      */
/* ------------------------------------------------------------------ */

START_TEST(test_execute_command_applies_assignments) {
  /* INTENDED behavior: a leading assignment on the Command should be
   * setenv'd for the child before exec. This will fail until
   * execute_command actually reads command->assignment_list. */
  char* out_path = exec_temp_file();
  Assignment* a = ass_new("FOO=bar");
  const char* argv[] = { "sh", "-c", "echo $FOO", NULL };
  Redirect redirs[] = { { .fd = 1, .mode = REDIR_OUT, .target = out_path } };
  Command* cmd = build_command(argv, a, 1, redirs);

  execute_command(cmd);
  char* content = read_whole_file(out_path);
  ck_assert_str_eq(content, "bar\n");

  free(content);
  unlink(out_path);
  free(out_path);
  command_delete(cmd);
}
END_TEST


Suite* exec_suite() {
  Suite* suite = suite_create("Execution Suite");
  TCase* short_circuit = tcase_create("Test Short Circuit");
  TCase* command_case = tcase_create("Test Command Execution");

  tcase_add_test(short_circuit, test_should_run_next_and_short_circuits_on_failure);
  tcase_add_test(short_circuit, test_should_run_next_or_short_circuits_on_success);
  tcase_add_test(command_case, test_exec_command_returns_success_status);
  tcase_add_test(command_case, test_exec_command_returns_failure_status);


  tcase_add_test(command_case, test_execute_command_success_status);
  tcase_add_test(command_case, test_execute_command_failure_status);
  tcase_add_test(command_case, test_execute_command_arbitrary_exit_code);
  tcase_add_test(command_case, test_execute_command_signal_terminated_status);
  tcase_add_test(command_case, test_execute_command_not_found_returns_127);
  tcase_add_test(command_case, test_execute_command_redirect_out_writes_file);
  tcase_add_test(command_case, test_execute_command_redirect_out_truncates_existing_content);
  tcase_add_test(command_case, test_execute_command_redirect_append_preserves_existing_content);
  tcase_add_test(command_case, test_execute_command_redirect_in_reads_file);
  tcase_add_test(command_case, test_execute_command_stderr_redirect);
  tcase_add_test(command_case, test_execute_command_heredoc_redirect_currently_unsupported);
  tcase_add_test(command_case, test_execute_command_input_redirect_bad_fd);
  tcase_add_test(command_case, test_execute_command_applies_assignments);

  suite_add_tcase(suite, short_circuit);
  suite_add_tcase(suite, command_case);

  return suite;
}