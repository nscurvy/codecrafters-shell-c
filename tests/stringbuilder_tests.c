//
// Created by nkinder on 9/12/26.
//

#include "check.h"

#include <stdio.h>
#include <stdlib.h>
#include "../src/stringbuilder.h"

#define assert(val) ck_assert((val))
#define streq(a, b) ck_assert_str_eq((a), (b))
#define uinteq(a, b) ck_assert_uint_eq((a), (b))
#define inteq(a, b) ck_assert_int_eq((a), (b))

START_TEST(test_sb_new_sized)
{
    StringBuilder *sb = sb_new_sized(10);
    fail_unless(sb != NULL, "Expected non-NULL StringBuilder");
    fail_unless(sb->capacity == 10, "Expected capacity to be 10");
    fail_unless(sb->size == 0, "Expected size to be 0");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appends)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appends(sb, "Hello");
    fail_unless(strcmp(sb->str, "Hello") == 0, "Expected 'Hello' but got %s", sb->str);
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appendc)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appendc(sb, 'A');
    fail_unless(sb->str[0] == 'A', "Expected 'A' but got %c", sb->str[0]);
    fail_unless(sb->size == 1, "Expected size to be 1");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appendl)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appendl(sb, 42);
    uinteq(sb->size, 2);
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_format)
{
    StringBuilder *sb = sb_new_sized(10);
    int result = sb_format(sb, "Hello %s!", "World");
    fail_unless(result > 0, "Expected positive return value from sb_format");
    fail_unless(strcmp(sb->str, "Hello World!") == 0, "Expected 'Hello World!' but got %s", sb->str);
    fail_unless(sb->size == 13, "Expected size to be 13");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_takestring)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appends(sb, "Hello");
    const char *result = sb_takestring(sb);
    fail_unless(result != NULL, "Expected non-NULL result from sb_takestring");
    fail_unless(strcmp(result, "Hello") == 0, "Expected 'Hello' but got %s", result);
    free((char *)result); // Free the allocated string
}
END_TEST

START_TEST(test_sb_new_sized_with_zero_capacity)
{
    StringBuilder *sb = sb_new_sized(0);
    fail_unless(sb != NULL, "Expected non-NULL StringBuilder");
    fail_unless(sb->capacity == 16, "Expected capacity to be 16 (default value)");
    fail_unless(sb->size == 0, "Expected size to be 0");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appends_with_empty_string)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appends(sb, "");
    fail_unless(strcmp(sb->str, "") == 0, "Expected empty string but got %s", sb->str);
    fail_unless(sb->size == 0, "Expected size to be 0");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appendc_with_space_character)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appendc(sb, ' ');
    fail_unless(sb->str[0] == ' ', "Expected space character but got %c", sb->str[0]);
    fail_unless(sb->size == 1, "Expected size to be 1");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_appendl_with_negative_number)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appendl(sb, -42);
    fail_unless(strcmp(sb->str, "-42") == 0, "Expected '-42' but got %s", sb->str);
    fail_unless(sb->size == 3, "Expected size to be 3");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_format_with_different_format_specifiers)
{
    StringBuilder *sb = sb_new_sized(10);
    int result = sb_format(sb, "%d %s %.2f", 42, "world", 3.14159);
    fail_unless(result > 0, "Expected positive return value from sb_format");
    fail_unless(strcmp(sb->str, "42 world 3.14") == 0, "Expected '42 world 3.14' but got %s", sb->str);
    fail_unless(sb->size == 14, "Expected size to be 14");
    sb_delete(sb);
}
END_TEST

START_TEST(test_sb_takestring_after_appending_multiple_strings)
{
    StringBuilder *sb = sb_new_sized(10);
    sb_appends(sb, "Hello ");
    sb_appends(sb, "World!");
    const char *result = sb_takestring(sb);
    fail_unless(result != NULL, "Expected non-NULL result from sb_takestring");
    fail_unless(strcmp(result, "Hello World!") == 0, "Expected 'Hello World!' but got %s", result);
    free((char *)result); // Free the allocated string
}
END_TEST


Suite* sb_suite() {
  Suite* suite = suite_create("String Builder Tests");
  TCase *sb_case = tcase_create("Test StringBuilder");
  TCase *sb_append_case = tcase_create("Test Append");

  tcase_add_test(sb_case,test_sb_new_sized);
  tcase_add_test(sb_case,test_sb_takestring);
  tcase_add_test(sb_append_case,test_sb_appendc);
  tcase_add_test(sb_append_case,test_sb_appendl);
  tcase_add_test(sb_append_case,test_sb_appends);
  tcase_add_test(sb_append_case,test_sb_format);
  tcase_add_test(sb_case, test_sb_new_sized_with_zero_capacity);
  tcase_add_test(sb_append_case, test_sb_appends_with_empty_string);
  tcase_add_test(sb_append_case, test_sb_appendc_with_space_character);
  tcase_add_test(sb_append_case, test_sb_appendl_with_negative_number);
  tcase_add_test(sb_append_case, test_sb_format_with_different_format_specifiers);
  tcase_add_test(sb_append_case, test_sb_takestring_after_appending_multiple_strings);

  suite_add_tcase(suite, sb_case);
  suite_add_tcase(suite, sb_append_case);
  return suite;
}
