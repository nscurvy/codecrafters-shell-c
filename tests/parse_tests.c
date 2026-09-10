//
// Created by nkinder on 9/10/26.
//
//
// Parser unit  tests
//

#include "check.h"

#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/parsetypes.h"
#include "../src/tokenlist.h"

#define assert(val) ck_assert((val))
#define streq(a, b) ck_assert_str_eq((a), (b))
#define uinteq(a, b) ck_assert_uint_eq((a), (b))
#define inteq(a, b) ck_assert_int_eq((a), (b))

static TokenList* tokenize_for_parser(const char* input) {
  CharStream* stream = cs_new(input);
  assert(stream);

  TokenList* list = tokenlist_new_empty();
  assert(list);

  int rc = lx_tokenize(list, stream);
  inteq(rc, 0);

  cs_delete(stream);
  return list;
}

static TokenStream* stream_from_input(const char* input, TokenList** out_tokens) {
  TokenList* tokens = tokenize_for_parser(input);
  assert(tokens);

  TokenStream* stream = ts_new(tokens);
  assert(stream);

  *out_tokens = tokens;
  return stream;
}

static void delete_stream_and_tokens(TokenStream* stream, TokenList* tokens) {
  ts_delete(stream);
  tokenlist_delete(tokens);
}

static Command* first_command_in_pipeline(Pipeline* pipeline) {
  assert(pipeline);
  assert(pipeline->head);
  assert(pipeline->head->command);
  return pipeline->head->command;
}

static Pipeline* first_pipeline_in_and_or(AndOr* and_or) {
  assert(and_or);
  assert(and_or->head);
  assert(and_or->head->pipeline);
  return and_or->head->pipeline;
}

static AndOr* first_and_or_in_list(List* list) {
  assert(list);
  assert(list->head);
  assert(list->head->and_or);
  return list->head->and_or;
}

/* ------------------------------------------------------------------ */
/* parse_command                                                       */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_command_single_word) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  assert(command->argv);
  streq(command->argv[0], "echo");
  ck_assert_ptr_null(command->argv[1]);
  uinteq(command->nredirs, 0);

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_multiple_args) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello world", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");
  streq(command->argv[2], "world");
  ck_assert_ptr_null(command->argv[3]);
  uinteq(command->nredirs, 0);

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_output_redirection) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello > out.txt", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");
  ck_assert_ptr_null(command->argv[2]);

  uinteq(command->nredirs, 1);
  inteq(command->redirs[0].fd, 1);
  inteq(command->redirs[0].mode, REDIR_OUT);
  streq(command->redirs[0].target, "out.txt");

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_input_redirection) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("cat < in.txt", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "cat");
  ck_assert_ptr_null(command->argv[1]);

  uinteq(command->nredirs, 1);
  inteq(command->redirs[0].fd, 0);
  inteq(command->redirs[0].mode, REDIR_IN);
  streq(command->redirs[0].target, "in.txt");

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_append_redirection) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello >> out.txt", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");
  ck_assert_ptr_null(command->argv[2]);

  uinteq(command->nredirs, 1);
  inteq(command->redirs[0].fd, 1);
  inteq(command->redirs[0].mode, REDIR_APPEND);
  streq(command->redirs[0].target, "out.txt");

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_explicit_fd_redirection) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("cmd 2> err.txt", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "cmd");
  ck_assert_ptr_null(command->argv[1]);

  uinteq(command->nredirs, 1);
  inteq(command->redirs[0].fd, 2);
  inteq(command->redirs[0].mode, REDIR_OUT);
  streq(command->redirs[0].target, "err.txt");

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_multiple_redirections) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("cmd < in.txt > out.txt", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "cmd");
  ck_assert_ptr_null(command->argv[1]);

  uinteq(command->nredirs, 2);

  inteq(command->redirs[0].fd, 0);
  inteq(command->redirs[0].mode, REDIR_IN);
  streq(command->redirs[0].target, "in.txt");

  inteq(command->redirs[1].fd, 1);
  inteq(command->redirs[1].mode, REDIR_OUT);
  streq(command->redirs[1].target, "out.txt");

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_assignment_before_command) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("FOO=bar echo hello", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");
  ck_assert_ptr_null(command->argv[2]);

  assert(command->assignment_list);
  streq(command->assignment_list->value, "FOO=bar");
  ck_assert_ptr_null(command->assignment_list->next);

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_multiple_assignments_before_command) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("FOO=bar BAZ=qux echo", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  ck_assert_ptr_null(command->argv[1]);

  assert(command->assignment_list);
  streq(command->assignment_list->value, "FOO=bar");
  assert(command->assignment_list->next);
  streq(command->assignment_list->next->value, "BAZ=qux");
  ck_assert_ptr_null(command->assignment_list->next->next);

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_command_assignment_after_command_is_argument) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo FOO=bar", &tokens);

  Command* command = parse_command(stream);

  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "FOO=bar");
  ck_assert_ptr_null(command->argv[2]);
  ck_assert_ptr_null(command->assignment_list);

  command_delete(command);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

/* ------------------------------------------------------------------ */
/* parse_pipeline                                                      */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_pipeline_single_command) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello", &tokens);

  Pipeline* pipeline = parse_pipeline(stream);

  assert(pipeline);
  uinteq(pipeline->size, 1);
  assert(pipeline->head);
  ck_assert_ptr_null(pipeline->head->next);

  Command* command = pipeline->head->command;
  assert(command);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");
  ck_assert_ptr_null(command->argv[2]);

  pipeline_delete(pipeline);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_pipeline_two_commands) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello | wc -c", &tokens);

  Pipeline* pipeline = parse_pipeline(stream);

  assert(pipeline);
  uinteq(pipeline->size, 2);
  assert(pipeline->head);
  assert(pipeline->head->next);
  ck_assert_ptr_null(pipeline->head->next->next);

  Command* first = pipeline->head->command;
  Command* second = pipeline->head->next->command;

  streq(first->argv[0], "echo");
  streq(first->argv[1], "hello");
  ck_assert_ptr_null(first->argv[2]);

  streq(second->argv[0], "wc");
  streq(second->argv[1], "-c");
  ck_assert_ptr_null(second->argv[2]);

  pipeline_delete(pipeline);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_pipeline_three_commands) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello | grep h | wc -l", &tokens);

  Pipeline* pipeline = parse_pipeline(stream);

  assert(pipeline);
  uinteq(pipeline->size, 3);

  PipelineElement* first = pipeline->head;
  PipelineElement* second = first->next;
  PipelineElement* third = second->next;

  assert(first);
  assert(second);
  assert(third);
  ck_assert_ptr_null(third->next);

  streq(first->command->argv[0], "echo");
  streq(first->command->argv[1], "hello");

  streq(second->command->argv[0], "grep");
  streq(second->command->argv[1], "h");

  streq(third->command->argv[0], "wc");
  streq(third->command->argv[1], "-l");

  pipeline_delete(pipeline);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

/* ------------------------------------------------------------------ */
/* parse_and_or                                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_and_or_single_pipeline) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello", &tokens);

  AndOr* and_or = parse_and_or(stream);

  assert(and_or);
  uinteq(and_or->count, 1);
  assert(and_or->head);
  inteq(and_or->head->op, AND_NONE);
  ck_assert_ptr_null(and_or->head->next);

  Command* command = first_command_in_pipeline(and_or->head->pipeline);
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");

  ao_delete(and_or);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_and_or_with_and_operator) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("true && echo ok", &tokens);

  AndOr* and_or = parse_and_or(stream);

  assert(and_or);
  uinteq(and_or->count, 2);
  assert(and_or->head);
  assert(and_or->head->next);

  inteq(and_or->head->op, AND_AND);
  inteq(and_or->head->next->op, AND_NONE);

  Command* first = first_command_in_pipeline(and_or->head->pipeline);
  Command* second = first_command_in_pipeline(and_or->head->next->pipeline);

  streq(first->argv[0], "true");
  streq(second->argv[0], "echo");
  streq(second->argv[1], "ok");

  ao_delete(and_or);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_and_or_with_or_operator) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("false || echo fallback", &tokens);

  AndOr* and_or = parse_and_or(stream);

  assert(and_or);
  uinteq(and_or->count, 2);
  assert(and_or->head);
  assert(and_or->head->next);

  inteq(and_or->head->op, AND_OR);
  inteq(and_or->head->next->op, AND_NONE);

  Command* first = first_command_in_pipeline(and_or->head->pipeline);
  Command* second = first_command_in_pipeline(and_or->head->next->pipeline);

  streq(first->argv[0], "false");
  streq(second->argv[0], "echo");
  streq(second->argv[1], "fallback");

  ao_delete(and_or);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_and_or_mixed_chain) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("false || true && echo done", &tokens);

  AndOr* and_or = parse_and_or(stream);

  assert(and_or);
  uinteq(and_or->count, 3);

  AndOrElement* first = and_or->head;
  AndOrElement* second = first->next;
  AndOrElement* third = second->next;

  assert(first);
  assert(second);
  assert(third);
  ck_assert_ptr_null(third->next);

  inteq(first->op, AND_OR);
  inteq(second->op, AND_AND);
  inteq(third->op, AND_NONE);

  streq(first_command_in_pipeline(first->pipeline)->argv[0], "false");
  streq(first_command_in_pipeline(second->pipeline)->argv[0], "true");
  streq(first_command_in_pipeline(third->pipeline)->argv[0], "echo");
  streq(first_command_in_pipeline(third->pipeline)->argv[1], "done");

  ao_delete(and_or);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

/* ------------------------------------------------------------------ */
/* parse_list                                                          */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_list_single_and_or) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo hello", &tokens);

  List* list = parse_list(stream);

  assert(list);
  uinteq(list->count, 1);
  assert(list->head);
  inteq(list->head->sep, SEP_NONE);
  ck_assert_ptr_null(list->head->next);

  Command* command = first_command_in_pipeline(first_pipeline_in_and_or(list->head->and_or));
  streq(command->argv[0], "echo");
  streq(command->argv[1], "hello");

  list_delete(list);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_list_semicolon_separator) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo one; echo two", &tokens);

  List* list = parse_list(stream);

  assert(list);
  uinteq(list->count, 2);
  assert(list->head);
  assert(list->head->next);

  inteq(list->head->sep, SEP_SEMI);
  inteq(list->head->next->sep, SEP_NONE);

  Command* first = first_command_in_pipeline(first_pipeline_in_and_or(list->head->and_or));
  Command* second = first_command_in_pipeline(first_pipeline_in_and_or(list->head->next->and_or));

  streq(first->argv[0], "echo");
  streq(first->argv[1], "one");

  streq(second->argv[0], "echo");
  streq(second->argv[1], "two");

  list_delete(list);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_list_ampersand_separator) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("sleep 1 & echo done", &tokens);

  List* list = parse_list(stream);

  assert(list);
  uinteq(list->count, 2);
  assert(list->head);
  assert(list->head->next);

  inteq(list->head->sep, SEP_AMP);
  inteq(list->head->next->sep, SEP_NONE);

  Command* first = first_command_in_pipeline(first_pipeline_in_and_or(list->head->and_or));
  Command* second = first_command_in_pipeline(first_pipeline_in_and_or(list->head->next->and_or));

  streq(first->argv[0], "sleep");
  streq(first->argv[1], "1");

  streq(second->argv[0], "echo");
  streq(second->argv[1], "done");

  list_delete(list);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

START_TEST(test_parse_list_trailing_semicolon) {
  TokenList* tokens = NULL;
  TokenStream* stream = stream_from_input("echo done;", &tokens);

  List* list = parse_list(stream);

  assert(list);
  uinteq(list->count, 1);
  assert(list->head);
  ck_assert_ptr_null(list->head->next);
  inteq(list->head->sep, SEP_SEMI);

  Command* command = first_command_in_pipeline(first_pipeline_in_and_or(list->head->and_or));
  streq(command->argv[0], "echo");
  streq(command->argv[1], "done");

  list_delete(list);
  delete_stream_and_tokens(stream, tokens);
}
END_TEST

Suite* parser_suite() {
  Suite* suite = suite_create("Parser");
  TCase* command_case = tcase_create("Parse Command");
  TCase* pipeline_case = tcase_create("Parse Pipeline");
  TCase* andor_case = tcase_create("Parse Andor");
  TCase* list_case = tcase_create("Parse List");

  tcase_add_test(command_case, test_parse_command_single_word);
  tcase_add_test(command_case, test_parse_command_multiple_args);
  tcase_add_test(command_case, test_parse_command_output_redirection);
  tcase_add_test(command_case, test_parse_command_input_redirection);
  tcase_add_test(command_case, test_parse_command_append_redirection);
  tcase_add_test(command_case, test_parse_command_explicit_fd_redirection);
  tcase_add_test(command_case, test_parse_command_multiple_redirections);
  tcase_add_test(command_case, test_parse_command_assignment_before_command);
  tcase_add_test(command_case, test_parse_command_multiple_assignments_before_command);
  tcase_add_test(command_case, test_parse_command_assignment_after_command_is_argument);

  tcase_add_test(pipeline_case, test_parse_pipeline_single_command);
  tcase_add_test(pipeline_case, test_parse_pipeline_two_commands);
  tcase_add_test(pipeline_case, test_parse_pipeline_three_commands);

  tcase_add_test(andor_case, test_parse_and_or_single_pipeline);
  tcase_add_test(andor_case, test_parse_and_or_with_and_operator);
  tcase_add_test(andor_case, test_parse_and_or_with_or_operator);
  tcase_add_test(andor_case, test_parse_and_or_mixed_chain);

  tcase_add_test(list_case, test_parse_list_single_and_or);
  tcase_add_test(list_case, test_parse_list_semicolon_separator);
  tcase_add_test(list_case, test_parse_list_ampersand_separator);
  tcase_add_test(list_case, test_parse_list_trailing_semicolon);

  suite_add_tcase(suite, command_case);
  suite_add_tcase(suite, pipeline_case);
  suite_add_tcase(suite, andor_case);
  suite_add_tcase(suite, list_case);

  return suite;
}