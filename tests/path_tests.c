//
// Created by nkinder on 9/13/26.
//

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "tests_common.h"
#include <check.h>

#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Fixture helpers                                                      */
/* ------------------------------------------------------------------ */

/* Creates a temp file, optionally executable, returns malloc'd path.
 * Caller must unlink() and free() it. */
static char* make_temp_file(bool executable) {
    char* path = strdup("/tmp/path_test_XXXXXX");
    int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);
    if (executable) {
        chmod(path, 0755);
    } else {
        chmod(path, 0644);
    }
    return path;
}

static char* make_temp_dir(void) {
    char* path = strdup("/tmp/path_test_dir_XXXXXX");
    char* result = mkdtemp(path);
    assert(result != NULL);
    return path;
}

/* ------------------------------------------------------------------ */
/* has_path_components                                                  */
/* ------------------------------------------------------------------ */

START_TEST(test_has_path_components_absolute_path) {
  assert(has_path_components("/bin/ls"));
}
END_TEST

START_TEST(test_has_path_components_relative_with_dot_slash) {
  assert(has_path_components("./foo"));
}
END_TEST

START_TEST(test_has_path_components_relative_with_dotdot_slash) {
  assert(has_path_components("../foo"));
}
END_TEST

START_TEST(test_has_path_components_bare_command_name) {
  assert(!has_path_components("ls"));
}
END_TEST

START_TEST(test_has_path_components_bare_name_with_dots_no_slash) {
  assert(!has_path_components("foo.sh"));
}
END_TEST

START_TEST(test_has_path_components_empty_string) {
  assert(!has_path_components(""));
}
END_TEST

/* ------------------------------------------------------------------ */
/* path_join                                                            */
/* ------------------------------------------------------------------ */

START_TEST(test_path_join_normal_dir) {
  char* result = path_join("/usr/bin", "ls");
  ck_assert_str_eq(result, "/usr/bin/ls");
  free(result);
}
END_TEST

START_TEST(test_path_join_dir_with_trailing_content) {
  char* result = path_join("/tmp", "myscript");
  ck_assert_str_eq(result, "/tmp/myscript");
  free(result);
}
END_TEST

START_TEST(test_path_join_empty_dir_uses_current_dir) {
  /* KNOWN BUG: current impl strcpy()s from `dir` (empty) instead of
   * `dir_actual` ("."), producing a malformed/empty result instead of
   * "./name". This test documents the intended behavior. */
  char* result = path_join("", "ls");
  ck_assert_str_eq(result, "./ls");
  free(result);
}
END_TEST

/* ------------------------------------------------------------------ */
/* path_is_executable_file                                              */
/* ------------------------------------------------------------------ */

START_TEST(test_path_is_executable_file_true_for_executable_regular_file) {
  char* path = make_temp_file(true);
  assert(path_is_executable_file(path));
  unlink(path);
  free(path);
}
END_TEST

START_TEST(test_path_is_executable_file_false_for_non_executable_file) {
  char* path = make_temp_file(false);
  assert(!path_is_executable_file(path));
  unlink(path);
  free(path);
}
END_TEST

START_TEST(test_path_is_executable_file_false_for_nonexistent_path) {
  assert(!path_is_executable_file("/tmp/path_test_definitely_does_not_exist_12345"));
}
END_TEST

START_TEST(test_path_is_executable_file_false_for_directory) {
  /* Directories can have the executable bit set but are not S_ISREG. */
  char* dir = make_temp_dir();
  chmod(dir, 0755);
  assert(!path_is_executable_file(dir));
  rmdir(dir);
  free(dir);
}
END_TEST

/* ------------------------------------------------------------------ */
/* path_split                                                           */
/* ------------------------------------------------------------------ */

START_TEST(test_path_split_empty_string_yields_empty_list) {
  WordList* list = path_split("");
  uinteq(list->size, 0);
  assert(list->head == NULL);
  wordlist_delete(list);
}
END_TEST

START_TEST(test_path_split_single_component_no_trailing_colon) {
  /* KNOWN BUG: current impl only appends a component when it hits ':',
   * so a single component with NO trailing colon is silently dropped.
   * This test documents the intended behavior (split still yields it). */
  WordList* list = path_split("/usr/bin");
  uinteq(list->size, 1);
  ck_assert_str_eq(list->head->text, "/usr/bin");
  wordlist_delete(list);
}
END_TEST

START_TEST(test_path_split_two_components_no_trailing_colon) {
  /* Same known bug -- the SECOND (last) component here would currently
   * be dropped since the string doesn't end in ':'. */
  WordList* list = path_split("/usr/bin:/bin");
  uinteq(list->size, 2);
  ck_assert_str_eq(list->head->text, "/usr/bin");
  ck_assert_str_eq(list->head->next->text, "/bin");
  wordlist_delete(list);
}
END_TEST

START_TEST(test_path_split_trailing_colon_still_works) {
  /* This shape already works under the current implementation, since it
   * only appends on seeing ':'. */
  WordList* list = path_split("/usr/bin:/bin:");
  uinteq(list->size, 2);
  ck_assert_str_eq(list->head->text, "/usr/bin");
  ck_assert_str_eq(list->head->next->text, "/bin");
  wordlist_delete(list);
}
END_TEST

START_TEST(test_path_split_three_components) {
  WordList* list = path_split("/usr/local/bin:/usr/bin:/bin");
  uinteq(list->size, 3);
  ck_assert_str_eq(list->head->text, "/usr/local/bin");
  ck_assert_str_eq(list->head->next->text, "/usr/bin");
  ck_assert_str_eq(list->head->next->next->text, "/bin");
  wordlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* path_getenv / path_get_dirs                                          */
/* ------------------------------------------------------------------ */

START_TEST(test_path_getenv_matches_real_getenv) {
  const char* expected = getenv("PATH");
  const char* actual = path_getenv();
  if (expected == NULL) {
      assert(actual == NULL);
  } else {
      ck_assert_str_eq(actual, expected);
  }
}
END_TEST

START_TEST(test_path_get_dirs_splits_current_path) {
  char* saved = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
  setenv("PATH", "/foo:/bar:/baz", 1);

  WordList* list = path_get_dirs();
  uinteq(list->size, 3);
  ck_assert_str_eq(list->head->text, "/foo");
  ck_assert_str_eq(list->head->next->text, "/bar");
  ck_assert_str_eq(list->head->next->next->text, "/baz");
  wordlist_delete(list);

  if (saved) { setenv("PATH", saved, 1); free(saved); } else { unsetenv("PATH"); }
}
END_TEST

START_TEST(test_path_get_dirs_handles_unset_path_without_crashing) {
  /* Regression test for the null-PATH crash found earlier: path_getenv()
   * returning NULL must not segfault path_split/path_get_dirs. */
  char* saved = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
  unsetenv("PATH");

  WordList* list = path_get_dirs();
  assert(list != NULL);
  uinteq(list->size, 0);
  wordlist_delete(list);

  if (saved) { setenv("PATH", saved, 1); free(saved); }
}
END_TEST

/* ------------------------------------------------------------------ */
/* path_find_command                                                    */
/* ------------------------------------------------------------------ */

START_TEST(test_path_find_command_absolute_executable_path) {
  char* path = make_temp_file(true);
  char* result = path_find_command(path);
  assert(result != NULL);
  ck_assert_str_eq(result, path);
  free(result);
  unlink(path);
  free(path);
}
END_TEST

START_TEST(test_path_find_command_absolute_non_executable_path) {
  char* path = make_temp_file(false);
  char* result = path_find_command(path);
  assert(result == NULL);
  unlink(path);
  free(path);
}
END_TEST

START_TEST(test_path_find_command_absolute_nonexistent_path) {
  char* result = path_find_command("/tmp/path_test_definitely_does_not_exist_12345");
  assert(result == NULL);
}
END_TEST

START_TEST(test_path_find_command_found_via_path) {
  char* dir = make_temp_dir();
  char* script_path = NULL;
  {
      size_t len = strlen(dir) + strlen("/myscript") + 1;
      script_path = malloc(len);
      snprintf(script_path, len, "%s/myscript", dir);
  }
  int fd = open(script_path, O_WRONLY | O_CREAT, 0755);
  assert(fd >= 0);
  close(fd);
  chmod(script_path, 0755);

  char* saved = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
  setenv("PATH", dir, 1);

  char* result = path_find_command("myscript");
  assert(result != NULL);
  ck_assert_str_eq(result, script_path);

  if (saved) { setenv("PATH", saved, 1); free(saved); } else { unsetenv("PATH"); }
  free(result);
  unlink(script_path);
  free(script_path);
  rmdir(dir);
  free(dir);
}
END_TEST

START_TEST(test_path_find_command_not_found_on_path) {
  char* dir = make_temp_dir();

  char* saved = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
  setenv("PATH", dir, 1);

  char* result = path_find_command("this_command_should_not_exist_anywhere");
  assert(result == NULL);

  if (saved) { setenv("PATH", saved, 1); free(saved); } else { unsetenv("PATH"); }
  rmdir(dir);
  free(dir);
}
END_TEST

START_TEST(test_path_find_command_searches_multiple_dirs_in_order) {
  char* dir1 = make_temp_dir();
  char* dir2 = make_temp_dir();

  /* Only dir2 has the script -- confirms the search continues past a
   * directory that doesn't contain it. */
  char* script_path = NULL;
  {
      size_t len = strlen(dir2) + strlen("/foundme") + 1;
      script_path = malloc(len);
      snprintf(script_path, len, "%s/foundme", dir2);
  }
  int fd = open(script_path, O_WRONLY | O_CREAT, 0755);
  assert(fd >= 0);
  close(fd);
  chmod(script_path, 0755);

  char* saved = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
  char* combined_path = NULL;
  {
      size_t len = strlen(dir1) + strlen(dir2) + 2;
      combined_path = malloc(len);
      snprintf(combined_path, len, "%s:%s", dir1, dir2);
  }
  setenv("PATH", combined_path, 1);

  char* result = path_find_command("foundme");
  assert(result != NULL);
  ck_assert_str_eq(result, script_path);

  if (saved) { setenv("PATH", saved, 1); free(saved); } else { unsetenv("PATH"); }
  free(result);
  free(combined_path);
  unlink(script_path);
  free(script_path);
  rmdir(dir1);
  rmdir(dir2);
  free(dir1);
  free(dir2);
}
END_TEST


Suite* path_suite() {
  Suite* suite = suite_create("Path Test Suite");
  TCase* components = tcase_create("Test Path Components");
  TCase* join = tcase_create("Test Path Join");
  TCase* executable = tcase_create("Test Is Executable");
  TCase* split = tcase_create("Test Path Split");
  TCase* env = tcase_create("Test GetEnv");
  TCase* get_dirs = tcase_create("Test Get Dirs");
  TCase* find = tcase_create("Test Find Command");

  tcase_add_test(components, test_has_path_components_absolute_path);
  tcase_add_test(components, test_has_path_components_relative_with_dot_slash);
  tcase_add_test(components, test_has_path_components_relative_with_dotdot_slash);
  tcase_add_test(components, test_has_path_components_bare_command_name);
  tcase_add_test(components, test_has_path_components_bare_name_with_dots_no_slash);
  tcase_add_test(components, test_has_path_components_empty_string);
  suite_add_tcase(suite, components);

  tcase_add_test(join, test_path_join_normal_dir);
  tcase_add_test(join, test_path_join_dir_with_trailing_content);
  tcase_add_test(join, test_path_join_empty_dir_uses_current_dir);
  suite_add_tcase(suite, join);

  tcase_add_test(executable, test_path_is_executable_file_true_for_executable_regular_file);
  tcase_add_test(executable, test_path_is_executable_file_false_for_non_executable_file);
  tcase_add_test(executable, test_path_is_executable_file_false_for_nonexistent_path);
  tcase_add_test(executable, test_path_is_executable_file_false_for_directory);
  suite_add_tcase(suite, executable);

  tcase_add_test(split, test_path_split_empty_string_yields_empty_list);
  tcase_add_test(split, test_path_split_single_component_no_trailing_colon);
  tcase_add_test(split, test_path_split_two_components_no_trailing_colon);
  tcase_add_test(split, test_path_split_trailing_colon_still_works);
  tcase_add_test(split, test_path_split_three_components);
  suite_add_tcase(suite, split);

  tcase_add_test(env, test_path_getenv_matches_real_getenv);
  suite_add_tcase(suite, env);

  tcase_add_test(get_dirs, test_path_get_dirs_splits_current_path);
  tcase_add_test(get_dirs, test_path_get_dirs_handles_unset_path_without_crashing);
  suite_add_tcase(suite, get_dirs);

  tcase_add_test(find, test_path_find_command_absolute_executable_path);
  tcase_add_test(find, test_path_find_command_absolute_non_executable_path);
  tcase_add_test(find, test_path_find_command_absolute_nonexistent_path);
  tcase_add_test(find, test_path_find_command_found_via_path);
  tcase_add_test(find, test_path_find_command_not_found_on_path);
  tcase_add_test(find, test_path_find_command_searches_multiple_dirs_in_order);
  suite_add_tcase(suite, find);

  return suite;
}