//
// Created by nkinder on 9/5/26.
//
#include "check.h"
#include "../src/common.h"
#include "../src/lexer.h"

#define assert(val) ck_assert((val))
#define streq(a, b) ck_assert_str_eq((a), (b))
#define uinteq(a,b) ck_assert_uint_eq((a),(b))
#define inteq(a,b) ck_assert_int_eq((a), (b))
START_TEST(test_char_stream) {
  const char* test_line = "echo \"some value\" | echo \"more\" 1>/dev/null";

  CharStream* stream = cs_new(test_line);
  assert(stream);
  int ch = cs_peek(stream);
  inteq(ch, 'e');
  ch = cs_read(stream);
  uinteq(stream->pos, 1);
  cs_delete(stream);


}
END_TEST

START_TEST(test_char_stream_on_eoi) {
  const char* line = "echo\n";
  CharStream* stream = cs_new(line);
  for (int i = 0; i < 5; ++i) {
    cs_read(stream);
  }

  int result = cs_read(stream);
  inteq(result, EOI);
  assert(cs_eoi(stream));
  uinteq(stream->pos, stream->len);
  cs_delete(stream);

}
END_TEST

START_TEST(test_token_type) {
  const char* line = "hello";
  CharStream* stream = cs_new(line);
  char* begin = nullptr;
  char* end = nullptr;
  TokenType result = lx_scan(stream, &begin, &end);
  inteq(result, TOK_WORD);
  streq(begin, line);
  uinteq(end - begin, (size_t)6);
  cs_delete(stream);
}
END_TEST

START_TEST(test_scan_on_multi_word) {
  const char* line = "hello world";
  CharStream* stream = cs_new(line);
  char* begin = nullptr;
  char* end = nullptr;
  TokenType result_a = lx_scan(stream, &begin, &end);
  inteq(result_a, TOK_WORD);
  uinteq(end - begin, 6);
  inteq(*begin, 'h');

  result_a = lx_scan(stream, &begin, &end);
  inteq(*begin, 'w');
  uinteq(end - begin, 6);
  cs_delete(stream);

}
END_TEST

START_TEST(test_scan_on_single_char_operator) {
  const char* line = ";";
  CharStream* stream = cs_new(line);
  char* begin = nullptr;
  char* end = nullptr;
  TokenType result_a = lx_scan(stream, &begin, &end);
  inteq(result_a, TOK_SEMI);
  uinteq(end - begin, 2);
  inteq(*begin, ';');
  cs_delete(stream);
}
END_TEST

START_TEST(test_scan_on_multi_char_operator) {
  const char* line = "<<";
  CharStream* stream = cs_new(line);
  char* begin = nullptr;
  char* end = nullptr;
  TokenType result_a = lx_scan(stream, &begin, &end);
  inteq(result_a, TOK_REDIR_HEREDOC);
  uinteq(end - begin, 3);
  inteq(*begin, '<');
  cs_delete(stream);
}
END_TEST

START_TEST(test_scan_on_multi_char_operator_with_word) {
  const char* line = "echo <<";
  CharStream* stream = cs_new(line);
  char* begin = nullptr;
  char* end = nullptr;
  TokenType result_a = lx_scan(stream, &begin, &end);
  inteq(result_a, TOK_WORD);
  result_a = lx_scan(stream, &begin, &end);


  inteq(result_a, TOK_REDIR_HEREDOC);
  uinteq(end - begin, 3);
  inteq(*begin, '<');
  cs_delete(stream);
}
END_TEST
START_TEST(test_lexer_tokenizes_echo) {
    const char* line   = "echo";
    CharStream* stream = cs_new(line);
    Token*      tok    = lx_token(stream);

    inteq(tok->type, TOK_WORD);
    streq(tok->value, "echo");
    token_delete(tok);
  cs_delete(stream);
}
END_TEST

Suite* lexer_suite(void) {
  Suite* s;
  TCase* tc_core;

  s = suite_create("Lexer");

  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_char_stream);
  tcase_add_test(tc_core, test_char_stream_on_eoi);
  tcase_add_test(tc_core, test_token_type);
  tcase_add_test(tc_core, test_scan_on_multi_word);
  tcase_add_test(tc_core, test_scan_on_single_char_operator);
  tcase_add_test(tc_core, test_scan_on_multi_char_operator);
  tcase_add_test(tc_core, test_scan_on_multi_char_operator_with_word);
  tcase_add_test(tc_core, test_lexer_tokenizes_echo);

  suite_add_tcase(s, tc_core);

  return s;
}
