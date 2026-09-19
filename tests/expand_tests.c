//
// Created by nkinder on 9/19/26.
//
#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"
#include "stringbuilder.h"
#include "tests_common.h"
#include "tokenlist.h"
#include "vars.h"

void
expand_variable(const char** i, StringBuilder* sb);
/* ------------------------------------------------------------------ */
/* expand_word -- literals, no expansion needed                        */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_word_plain_literal_unchanged) {
    const char* result = expand_word("hello");
    ck_assert_str_eq(result, "hello");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_empty_string) {
    const char* result = expand_word("");
    ck_assert_str_eq(result, "");
    free((void*) result);
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_word -- variable expansion                                   */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_word_simple_variable) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("$EXPAND_TEST_FOO");
    ck_assert_str_eq(result, "bar");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_variable_glued_to_following_literal) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("$EXPAND_TEST_FOO/suffix");
    ck_assert_str_eq(result, "bar/suffix");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_variable_glued_to_preceding_literal) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("prefix=$EXPAND_TEST_FOO");
    ck_assert_str_eq(result, "prefix=bar");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_unset_variable_expands_to_empty) {
    var_unassign("EXPAND_TEST_DEFINITELY_UNSET");
    const char* result = expand_word("$EXPAND_TEST_DEFINITELY_UNSET");
    ck_assert_str_eq(result, "");
    free((void*) result);
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_word -- quoting                                              */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_word_single_quotes_removed) {
    const char* result = expand_word("'hello world'");
    ck_assert_str_eq(result, "hello world");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_single_quotes_prevent_variable_expansion) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("'$EXPAND_TEST_FOO'");
    ck_assert_str_eq(result, "$EXPAND_TEST_FOO"); /* literal, not expanded */
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_double_quotes_removed) {
    const char* result = expand_word("\"hello world\"");
    ck_assert_str_eq(result, "hello world");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_double_quotes_allow_variable_expansion) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("\"$EXPAND_TEST_FOO\"");
    ck_assert_str_eq(result, "bar");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_glued_quoted_and_expanded_segments) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("'a'$EXPAND_TEST_FOO");
    ck_assert_str_eq(result, "abar");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_word -- backslash escapes                                    */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_word_unquoted_backslash_dollar_is_literal) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("\\$EXPAND_TEST_FOO");
    ck_assert_str_eq(result, "$EXPAND_TEST_FOO"); /* escaped -- not expanded */
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_double_quoted_backslash_dollar_is_literal) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char* result = expand_word("\"\\$EXPAND_TEST_FOO\"");
    ck_assert_str_eq(result, "$EXPAND_TEST_FOO");
    free((void*) result);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_word_backslash_inside_single_quotes_is_literal) {
    const char* result = expand_word("'a\\b'");
    ck_assert_str_eq(result, "a\\b"); /* backslash has no meaning in single quotes */
    free((void*) result);
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_word -- tilde expansion (ASSUMED: leading-position only)      */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_word_bare_tilde_expands_to_home) {
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("~");
    ck_assert_str_eq(result, "/home/testuser");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_leading_tilde_with_path) {
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("~/projects");
    ck_assert_str_eq(result, "/home/testuser/projects");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_mid_word_tilde_not_expanded) {
    /* ASSUMPTION: tilde expansion only applies at the start of a word. */
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("foo~bar");
    ck_assert_str_eq(result, "foo~bar");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_escaped_tilde_not_expanded) {
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("\\~");
    ck_assert_str_eq(result, "~");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_single_quoted_tilde_not_expanded) {
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("'~'");
    ck_assert_str_eq(result, "~");
    free((void*) result);
}
END_TEST

START_TEST(test_expand_word_double_quoted_tilde_not_expanded) {
    /* ASSUMPTION: tilde expansion is suppressed inside double quotes too
     * (standard POSIX behavior -- unconfirmed for this impl). */
    var_assign("HOME", "/home/testuser");
    const char* result = expand_word("\"~\"");
    ck_assert_str_eq(result, "~");
    free((void*) result);
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_token                                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_token_replaces_value_in_place) {
    var_assign("EXPAND_TEST_FOO", "bar");
    Token* tok = newtok(TOK_WORD, "$EXPAND_TEST_FOO");
    assert(tok != NULL);

    expand_token(tok);
    ck_assert_str_eq(tok->value, "bar");

    token_delete(tok);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_token_plain_literal_is_noop) {
    Token* tok = newtok(TOK_WORD, "plain");
    assert(tok != NULL);

    expand_token(tok);
    ck_assert_str_eq(tok->value, "plain");

    token_delete(tok);
}
END_TEST

START_TEST(test_expand_token_tilde) {
    var_assign("HOME", "/home/testuser");
    Token* tok = newtok(TOK_WORD, "~");
    assert(tok != NULL);

    expand_token(tok);
    ck_assert_str_eq(tok->value, "/home/testuser");

    token_delete(tok);
}
END_TEST

START_TEST(test_expand_token_does_not_change_type) {
    Token* tok = newtok(TOK_WORD, "plain");
    assert(tok != NULL);
    expand_token(tok);
    inteq(tok->type, TOK_WORD);
    token_delete(tok);
}
END_TEST

/* ------------------------------------------------------------------ */
/* expand_variable -- direct cursor-based calls                        */
/* ASSUMPTION: `i` points at the char right after '$' on entry, and is  */
/* left pointing just past the consumed variable name on return.       */
/* ------------------------------------------------------------------ */

START_TEST(test_expand_variable_simple_lookup) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char*    s  = "$EXPAND_TEST_FOO/rest";
    const char*    i  = s;
    StringBuilder* sb = sb_new();

    expand_variable(&i, sb);

    ck_assert_str_eq(sb->str, "bar");
    ck_assert_str_eq(i, "/rest"); /* cursor left right after the var name */

    sb_delete(sb);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_variable_unset_appends_nothing) {
    var_unassign("EXPAND_TEST_UNSET_XYZ");
    const char*    s  = "$EXPAND_TEST_UNSET_XYZ tail";
    const char*    i  = s;
    StringBuilder* sb = sb_new();

    expand_variable(&i, sb);

    ck_assert_str_eq(sb->str, "");
    ck_assert_str_eq(i, " tail");

    sb_delete(sb);
}
END_TEST

START_TEST(test_expand_variable_name_terminated_by_end_of_string) {
    var_assign("EXPAND_TEST_FOO", "bar");
    const char*    s  = "$EXPAND_TEST_FOO";
    const char*    i  = s;
    StringBuilder* sb = sb_new();

    expand_variable(&i, sb);

    ck_assert_str_eq(sb->str, "bar");
    ck_assert_str_eq(i, ""); /* cursor at the terminating nul */

    sb_delete(sb);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST

START_TEST(test_expand_variable_appends_to_existing_sb_content) {
    /* Confirms expand_variable APPENDS rather than overwriting whatever's
     * already in the StringBuilder -- relevant since expand_word calls it
     * mid-stream while building up a larger result. */
    var_assign("EXPAND_TEST_FOO", "bar");
    const char*    s  = "$EXPAND_TEST_FOO";
    const char*    i  = s;
    StringBuilder* sb = sb_new();
    sb_appends(sb, "prefix-");

    expand_variable(&i, sb);

    ck_assert_str_eq(sb->str, "prefix-bar");

    sb_delete(sb);
    var_unassign("EXPAND_TEST_FOO");
}
END_TEST


Suite*
expand_suite() {
    Suite* suite      = suite_create("Expansion tests");
    TCase* word_case  = tcase_create("Expand word");
    TCase* token_case = tcase_create("Expand token");
    TCase* var_case   = tcase_create("Expand variable");


    tcase_add_test(word_case, test_expand_word_plain_literal_unchanged);
    tcase_add_test(word_case, test_expand_word_empty_string);
    tcase_add_test(word_case, test_expand_word_simple_variable);
    tcase_add_test(word_case, test_expand_word_variable_glued_to_following_literal);
    tcase_add_test(word_case, test_expand_word_variable_glued_to_preceding_literal);
    tcase_add_test(word_case, test_expand_word_unset_variable_expands_to_empty);
    tcase_add_test(word_case, test_expand_word_single_quotes_removed);
    tcase_add_test(word_case, test_expand_word_single_quotes_prevent_variable_expansion);
    tcase_add_test(word_case, test_expand_word_double_quotes_removed);
    tcase_add_test(word_case, test_expand_word_double_quotes_allow_variable_expansion);
    tcase_add_test(word_case, test_expand_word_glued_quoted_and_expanded_segments);
    tcase_add_test(word_case, test_expand_word_unquoted_backslash_dollar_is_literal);
    tcase_add_test(word_case, test_expand_word_double_quoted_backslash_dollar_is_literal);
    tcase_add_test(word_case, test_expand_word_backslash_inside_single_quotes_is_literal);
    tcase_add_test(word_case, test_expand_word_bare_tilde_expands_to_home);
    tcase_add_test(word_case, test_expand_word_leading_tilde_with_path);
    tcase_add_test(word_case, test_expand_word_mid_word_tilde_not_expanded);
    tcase_add_test(word_case, test_expand_word_escaped_tilde_not_expanded);
    tcase_add_test(word_case, test_expand_word_single_quoted_tilde_not_expanded);
    tcase_add_test(word_case, test_expand_word_double_quoted_tilde_not_expanded);
    tcase_add_test(token_case, test_expand_token_replaces_value_in_place);
    tcase_add_test(token_case, test_expand_token_plain_literal_is_noop);
    tcase_add_test(token_case, test_expand_token_tilde);
    tcase_add_test(token_case, test_expand_token_does_not_change_type);
    tcase_add_test(var_case, test_expand_variable_simple_lookup);
    tcase_add_test(var_case, test_expand_variable_unset_appends_nothing);
    tcase_add_test(var_case, test_expand_variable_name_terminated_by_end_of_string);
    tcase_add_test(var_case, test_expand_variable_appends_to_existing_sb_content);

    suite_add_tcase(suite, word_case);
    suite_add_tcase(suite, token_case);
    suite_add_tcase(suite, var_case);

    return suite;
}
