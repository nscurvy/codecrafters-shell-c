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
  uinteq(end - begin, (size_t)5);
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
  uinteq(end - begin, 5);
  inteq(*begin, 'h');

  result_a = lx_scan(stream, &begin, &end);
  inteq(*begin, 'w');
  uinteq(end - begin, 5);
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
  uinteq(end - begin, 1);
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
  uinteq(end - begin, 2);
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
  uinteq(end - begin, 2);
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

/* Lexer / CharStream test cases.
 *
 * Assumes (per project convention):
 *   assert  -> ck_assert
 *   inteq   -> ck_assert_int_eq
 *   uinteq  -> ck_assert_uint_eq
 *
 * Two small static helpers below (tokenize_str / nth_token) just remove
 * repetitive boilerplate around lx_tokenize + walking the ->next chain --
 * same spirit as your inteq/uinteq aliases, not Check machinery.
 */

static TokenList* tokenize_str(const char* input) {
    CharStream* stream = cs_new(input);
    assert(stream);
    TokenList* list = tokenlist_new_empty();
    assert(list);
    int rc = lx_tokenize(list, stream);
    inteq(rc, 0);
    cs_delete(stream);
    return list;
}

static Token* nth_token(TokenList* list, size_t n) {
    Token* iter = list->head;
    for (size_t i = 0; i < n && iter != NULL; ++i) {
        iter = iter->next;
    }
    return iter;
}

/* ------------------------------------------------------------------ */
/* CharStream primitives                                               */
/* ------------------------------------------------------------------ */

START_TEST(test_cs_peek_does_not_advance) {
  CharStream* stream = cs_new("ab");
  int ch = cs_peek(stream);
  inteq(ch, 'a');
  ch = cs_peek(stream);
  inteq(ch, 'a'); /* still 'a' -- peek shouldn't move pos */
  uinteq(stream->pos, 0);
  cs_delete(stream);
}
END_TEST

START_TEST(test_cs_read_advances) {
  CharStream* stream = cs_new("ab");
  int ch = cs_read(stream);
  inteq(ch, 'a');
  uinteq(stream->pos, 1);
  ch = cs_read(stream);
  inteq(ch, 'b');
  uinteq(stream->pos, 2);
  assert(cs_eoi(stream));
  cs_delete(stream);
}
END_TEST

START_TEST(test_cs_peek_ahead) {
  CharStream* stream = cs_new("abc");
  inteq(cs_peek_ahead(stream, 0), 'a');
  inteq(cs_peek_ahead(stream, 1), 'b');
  inteq(cs_peek_ahead(stream, 2), 'c');
  uinteq(stream->pos, 0); /* lookahead shouldn't move pos either */
  cs_delete(stream);
}
END_TEST

START_TEST(test_cs_match_advances_on_success) {
  CharStream* stream = cs_new(">>");
  bool matched = cs_match(stream, '>');
  assert(matched);
  uinteq(stream->pos, 1);
  inteq(cs_peek(stream), '>');
  cs_delete(stream);
}
END_TEST

START_TEST(test_cs_match_does_not_advance_on_failure) {
  CharStream* stream = cs_new("ab");
  bool matched = cs_match(stream, 'x');
  assert(!matched);
  uinteq(stream->pos, 0);
  cs_delete(stream);
}
END_TEST

START_TEST(test_cs_eoi_on_empty_stream) {
  CharStream* stream = cs_new("");
  assert(cs_eoi(stream));
  cs_delete(stream);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Basic word tokenizing                                               */
/* ------------------------------------------------------------------ */

START_TEST(test_empty_input_yields_only_eof) {
  TokenList* list = tokenize_str("");
  uinteq(tokenlist_count(list), 1);
  inteq(nth_token(list, 0)->type, TOK_EOF);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_single_word) {
  TokenList* list = tokenize_str("echo");
  uinteq(tokenlist_count(list), 2);
  inteq(nth_token(list, 0)->type, TOK_WORD);
  ck_assert_str_eq(nth_token(list, 0)->value, "echo");
  inteq(nth_token(list, 1)->type, TOK_EOF);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_multiple_words_split_on_whitespace) {
  TokenList* list = tokenize_str("echo hello world");
  uinteq(tokenlist_count(list), 4);
  ck_assert_str_eq(nth_token(list, 0)->value, "echo");
  ck_assert_str_eq(nth_token(list, 1)->value, "hello");
  ck_assert_str_eq(nth_token(list, 2)->value, "world");
  inteq(nth_token(list, 3)->type, TOK_EOF);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_extra_whitespace_collapses) {
  TokenList* list = tokenize_str("  echo    hello  ");
  uinteq(tokenlist_count(list), 3);
  ck_assert_str_eq(nth_token(list, 0)->value, "echo");
  ck_assert_str_eq(nth_token(list, 1)->value, "hello");
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Quoting                                                             */
/* ASSUMPTION: quotes are preserved verbatim in the token's raw text   */
/* (quote removal happens later, during expansion).                   */
/* ------------------------------------------------------------------ */

START_TEST(test_single_quoted_word_preserves_quotes_and_spaces) {
  TokenList* list = tokenize_str("'hello world'");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "'hello world'");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_double_quoted_word_preserves_quotes_and_spaces) {
  TokenList* list = tokenize_str("\"hello world\"");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "\"hello world\"");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_quote_does_not_end_word_when_glued_to_more_text) {
  /* 'hello'world -> ONE token, not two. */
  TokenList* list = tokenize_str("'hello'world");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "'hello'world");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_single_quotes_protect_special_chars) {
  /* '|' inside single quotes must not become TOK_PIPE. */
  TokenList* list = tokenize_str("'a|b'");
  uinteq(tokenlist_count(list), 2);
  inteq(nth_token(list, 0)->type, TOK_WORD);
  ck_assert_str_eq(nth_token(list, 0)->value, "'a|b'");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_unterminated_single_quote_is_an_error) {
  CharStream* stream = cs_new("'unterminated");
  TokenList* list = tokenlist_new_empty();
  int rc = lx_tokenize(list, stream);
  assert(rc != 0);
  cs_delete(stream);
  tokenlist_delete(list);
}
END_TEST


/* ------------------------------------------------------------------ */
/* Backslash escapes                                                   */
/* ------------------------------------------------------------------ */

START_TEST(test_escaped_space_does_not_split_word) {
  /* hello\ world -> ONE token; backslash preserved (raw, unexpanded). */
  TokenList* list = tokenize_str("hello\\ world");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "hello\\ world");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_backslash_inside_single_quotes_is_literal) {
  TokenList* list = tokenize_str("'a\\b'");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "'a\\b'");
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Pipe and redirection                                                */
/* ------------------------------------------------------------------ */

START_TEST(test_pipe_operator) {
  TokenList* list = tokenize_str("echo hi | wc -l");
  uinteq(tokenlist_count(list), 6);
  ck_assert_str_eq(nth_token(list, 0)->value, "echo");
  ck_assert_str_eq(nth_token(list, 1)->value, "hi");
  inteq(nth_token(list, 2)->type, TOK_PIPE);
  ck_assert_str_eq(nth_token(list, 3)->value, "wc");
  ck_assert_str_eq(nth_token(list, 4)->value, "-l");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_pipe_without_surrounding_whitespace) {
  TokenList* list = tokenize_str("hi|wc");
  uinteq(tokenlist_count(list), 4);
  ck_assert_str_eq(nth_token(list, 0)->value, "hi");
  inteq(nth_token(list, 1)->type, TOK_PIPE);
  ck_assert_str_eq(nth_token(list, 2)->value, "wc");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_redir_out_default_fd_is_stdout) {
  TokenList* list = tokenize_str("echo hi > out.txt");
  uinteq(tokenlist_count(list), 5);
  Token* redir = nth_token(list, 2);
  inteq(redir->type, TOK_REDIR_OUT);
  inteq(redir->fd, 1);
  ck_assert_str_eq(nth_token(list, 3)->value, "out.txt");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_redir_in_default_fd_is_stdin) {
  TokenList* list = tokenize_str("wc -l < in.txt");
  Token* redir = nth_token(list, 2);
  inteq(redir->type, TOK_REDIR_IN);
  inteq(redir->fd, 0);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_redir_append_operator) {
  TokenList* list = tokenize_str("echo hi >> out.txt");
  Token* redir = nth_token(list, 2);
  inteq(redir->type, TOK_REDIR_APPEND);
  inteq(redir->fd, 1);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_redir_with_explicit_fd_prefix) {
  /* 2> err.txt -- leading digit sets the fd. */
  TokenList* list = tokenize_str("cmd 2> err.txt");
  Token* redir = nth_token(list, 1);
  inteq(redir->type, TOK_REDIR_OUT);
  inteq(redir->fd, 2);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_redir_glued_to_filename) {
  TokenList* list = tokenize_str("cmd 2>file");
  Token* redir = nth_token(list, 1);
  inteq(redir->type, TOK_REDIR_OUT);
  inteq(redir->fd, 2);
  ck_assert_str_eq(nth_token(list, 2)->value, "file");
  tokenlist_delete(list);
}
END_TEST
/* ------------------------------------------------------------------ */
/* &&, ||, ;, &                                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_and_operator) {
  TokenList* list = tokenize_str("true && false");
  uinteq(tokenlist_count(list), 4);
  inteq(nth_token(list, 1)->type, TOK_AND);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_or_operator) {
  TokenList* list = tokenize_str("true || false");
  inteq(nth_token(list, 1)->type, TOK_OR);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_single_pipe_is_not_confused_with_or) {
  TokenList* list = tokenize_str("a | b");
  inteq(nth_token(list, 1)->type, TOK_PIPE);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_semicolon_operator) {
  TokenList* list = tokenize_str("echo hi; echo bye");
  uinteq(tokenlist_count(list), 6);
  inteq(nth_token(list, 2)->type, TOK_SEMI);
  ck_assert_str_eq(nth_token(list, 3)->value, "echo");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_ampersand_operator) {
  TokenList* list = tokenize_str("sleep 5 &");
  uinteq(tokenlist_count(list), 4);
  inteq(nth_token(list, 2)->type, TOK_AMP);
  inteq(nth_token(list, 3)->type, TOK_EOF);
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Parens / braces                                                     */
/* ------------------------------------------------------------------ */

START_TEST(test_parens_are_their_own_tokens) {
  TokenList* list = tokenize_str("( echo hi )");
  uinteq(tokenlist_count(list), 5);
  inteq(nth_token(list, 0)->type, TOK_LPAREN);
  ck_assert_str_eq(nth_token(list, 1)->value, "echo");
  ck_assert_str_eq(nth_token(list, 2)->value, "hi");
  inteq(nth_token(list, 3)->type, TOK_RPAREN);
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_braces_are_their_own_tokens) {
  TokenList* list = tokenize_str("{ echo hi ; }");
  inteq(nth_token(list, 0)->type, TOK_LBRACE);
  inteq(nth_token(list, 3)->type, TOK_SEMI);
  inteq(nth_token(list, 4)->type, TOK_RBRACE);
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Newlines                                                            */
/* ------------------------------------------------------------------ */

START_TEST(test_newline_is_its_own_token) {
  TokenList* list = tokenize_str("echo hi\necho bye");
  uinteq(tokenlist_count(list), 6);
  inteq(nth_token(list, 2)->type, TOK_NEWLINE);
  ck_assert_str_eq(nth_token(list, 3)->value, "echo");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_newline_inside_single_quotes_is_literal) {
  TokenList* list = tokenize_str("'a\nb'");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "'a\nb'");
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Command substitution -- stays inside one raw TOK_WORD               */
/* If your lexer instead emits TOK_CMDSUB_START/END at the top level,  */
/* these three will need rewriting to match -- that's a real design    */
/* question worth revisiting, not just a test tweak.                   */
/* ------------------------------------------------------------------ */

START_TEST(test_cmdsub_glued_to_prefix_stays_one_word) {
  TokenList* list = tokenize_str("hel$(echo lo) world");
  uinteq(tokenlist_count(list), 3);
  ck_assert_str_eq(nth_token(list, 0)->value, "hel$(echo lo)");
  ck_assert_str_eq(nth_token(list, 1)->value, "world");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_cmdsub_nested_parens_do_not_end_word_early) {
  TokenList* list = tokenize_str("$(echo $(whoami))");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "$(echo $(whoami))");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_backtick_cmdsub_stays_one_word) {
  TokenList* list = tokenize_str("`echo hi`");
  uinteq(tokenlist_count(list), 2);
  ck_assert_str_eq(nth_token(list, 0)->value, "`echo hi`");
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Assignment words                                                    */
/* ------------------------------------------------------------------ */

START_TEST(test_leading_assignment_word) {
  TokenList* list = tokenize_str("FOO=bar echo hi");
  uinteq(tokenlist_count(list), 4);
  inteq(nth_token(list, 0)->type, TOK_ASSIGNMENT_WORD);
  ck_assert_str_eq(nth_token(list, 0)->value, "FOO=bar");
  ck_assert_str_eq(nth_token(list, 1)->value, "echo");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_equals_sign_after_command_word_is_lexed_as_assignment_shape) {
  /* The LEXER classifies by shape alone, regardless of position -- same
   * principle as reserved words. Whether "FOO=bar" here actually behaves
   * as an assignment (vs. a literal argument to echo) is a POSITIONAL
   * decision the PARSER makes later, not something the lexer can or
   * should determine. */
  TokenList* list = tokenize_str("echo FOO=bar");
  inteq(nth_token(list, 0)->type, TOK_WORD);
  inteq(nth_token(list, 1)->type, TOK_ASSIGNMENT_WORD);
  ck_assert_str_eq(nth_token(list, 1)->value, "FOO=bar");
  tokenlist_delete(list);
}
END_TEST

/* ------------------------------------------------------------------ */
/* Reserved words -- lexer should emit plain TOK_WORD; reclassifying   */
/* to TOK_RESERVED_WORD based on grammar position is the parser's job. */
/* ------------------------------------------------------------------ */

START_TEST(test_reserved_word_text_is_lexed_as_plain_word) {
  TokenList* list = tokenize_str("if");
  inteq(nth_token(list, 0)->type, TOK_WORD);
  ck_assert_str_eq(nth_token(list, 0)->value, "if");
  tokenlist_delete(list);
}
END_TEST

START_TEST(test_reserved_word_as_argument_is_just_a_word) {
  TokenList* list = tokenize_str("echo if");
  ck_assert_str_eq(nth_token(list, 0)->value, "echo");
  inteq(nth_token(list, 1)->type, TOK_WORD);
  ck_assert_str_eq(nth_token(list, 1)->value, "if");
  tokenlist_delete(list);
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

  tcase_add_test(tc_core, test_cs_peek_does_not_advance);
  tcase_add_test(tc_core, test_cs_read_advances);
  tcase_add_test(tc_core, test_cs_peek_ahead);
  tcase_add_test(tc_core, test_cs_match_advances_on_success);
  tcase_add_test(tc_core, test_cs_match_does_not_advance_on_failure);
  tcase_add_test(tc_core, test_cs_eoi_on_empty_stream);
  tcase_add_test(tc_core, test_empty_input_yields_only_eof);
  tcase_add_test(tc_core, test_single_word);
  tcase_add_test(tc_core, test_multiple_words_split_on_whitespace);
  tcase_add_test(tc_core, test_extra_whitespace_collapses);
  tcase_add_test(tc_core, test_single_quoted_word_preserves_quotes_and_spaces);
  tcase_add_test(tc_core, test_double_quoted_word_preserves_quotes_and_spaces);
  tcase_add_test(tc_core, test_quote_does_not_end_word_when_glued_to_more_text);
  tcase_add_test(tc_core, test_single_quotes_protect_special_chars);
  tcase_add_test(tc_core, test_unterminated_single_quote_is_an_error);
  tcase_add_test(tc_core, test_escaped_space_does_not_split_word);
  tcase_add_test(tc_core, test_backslash_inside_single_quotes_is_literal);
  tcase_add_test(tc_core, test_pipe_operator);
  tcase_add_test(tc_core, test_pipe_without_surrounding_whitespace);
  tcase_add_test(tc_core, test_redir_out_default_fd_is_stdout);
  tcase_add_test(tc_core, test_redir_in_default_fd_is_stdin);
  tcase_add_test(tc_core, test_redir_append_operator);
  tcase_add_test(tc_core, test_redir_with_explicit_fd_prefix);
  tcase_add_test(tc_core, test_redir_glued_to_filename);
  tcase_add_test(tc_core, test_and_operator);
  tcase_add_test(tc_core, test_or_operator);
  tcase_add_test(tc_core, test_single_pipe_is_not_confused_with_or);
  tcase_add_test(tc_core, test_semicolon_operator);
  tcase_add_test(tc_core, test_ampersand_operator);
  tcase_add_test(tc_core, test_parens_are_their_own_tokens);
  tcase_add_test(tc_core, test_braces_are_their_own_tokens);
  tcase_add_test(tc_core, test_newline_is_its_own_token);
  tcase_add_test(tc_core, test_newline_inside_single_quotes_is_literal);
  tcase_add_test(tc_core, test_cmdsub_glued_to_prefix_stays_one_word);
  tcase_add_test(tc_core, test_cmdsub_nested_parens_do_not_end_word_early);
  tcase_add_test(tc_core, test_backtick_cmdsub_stays_one_word);
  tcase_add_test(tc_core, test_leading_assignment_word);
  tcase_add_test(tc_core, test_equals_sign_after_command_word_is_lexed_as_assignment_shape);
  tcase_add_test(tc_core, test_reserved_word_text_is_lexed_as_plain_word);
  tcase_add_test(tc_core, test_reserved_word_as_argument_is_just_a_word);


  suite_add_tcase(s, tc_core);

  return s;
}
