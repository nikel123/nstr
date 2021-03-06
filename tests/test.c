#include <stdio.h>
#include <nstr.h>

#undef NDEBUG
#include <assert.h>

int
main(
    int  argc,
    char **argv) {

  int ret = 0;
  const char *str;

  nstr_t *s1 = nstr_new_cstr("str1");
  assert(s1);
  assert(s1->type == NSTR_CSTR_T);
  assert(s1->len == 4);
  assert(strcmp(s1->cstr, "str1") == 0);

  nstr_t *s2 = nstr_new_const("str2", 4);
  assert(s2);
  assert(s2->type == NSTR_CONST_T);
  assert(s2->len == 4);

  char *s3str = strdup("str3");
  assert(s3str);
  nstr_t *s3 =
    nstr_new_buffer(
      s3str, s3str, 4, 5);
  assert(s3);
  assert(s3->type == NSTR_BUFFER_CSTR_T);
  assert(s3->len == 4);
  assert(strcmp(s3->buf.str, "str3") == 0);

  char *s4str = strdup("str4X");
  assert(s4str);
  nstr_t *s4 =
    nstr_new_buffer(
      s4str, s4str, 4, 5);
  assert(s4);
  assert(s4->type == NSTR_BUFFER_CSTR_T);
  assert(s4->len == 4);
  assert(s4->buf.len == 5);
  assert(strcmp(s4->buf.str, "str4") == 0);

  char *s5str = strdup("Xstr5");
  assert(s5str);
  nstr_t *s5 =
    nstr_new_buffer(
       s5str, &(s5str[1]), 4, 6);
  assert(s5);
  assert(s5->type == NSTR_BUFFER_CSTR_T);
  assert(s5->len == 4);
  assert(s5->buf.len == 6);
  assert(strcmp(s5->buf.str, "str5") == 0);

  char *s6str = strdup("Xstr6X");
  assert(s6str);
  nstr_t *s6 =
    nstr_new_buffer(
      s6str, &(s6str[1]), 4, 7);
  assert(s6);
  assert(s6->type == NSTR_BUFFER_CSTR_T);
  assert(s6->len == 4);
  assert(s6->buf.len == 7);
  assert(strcmp(s6->buf.str, "str6") == 0);

  char *s7str = malloc(4);
  assert(s7str);
  memcpy(s7str, "str7", 4);
  nstr_t *s7 =
    nstr_new_buffer(
      s7str, s7str, 4, 4);
  assert(s7);
  assert(s7->type == NSTR_BUFFER_T);
  assert(s7->len == 4);
  assert(s7->buf.len == 4);

  char *s8str = malloc(5);
  assert(s8str);
  memcpy(s8str, "Xstr8", 5);
  nstr_t *s8 =
    nstr_new_buffer(
      s8str, &(s8str[1]), 4, 5);
  assert(s8);
  assert(s8->type == NSTR_BUFFER_T);
  assert(s8->len == 4);
  assert(s8->buf.len == 5);

  nstr_t *s9ref = nstr_new_cstr("Xstr9");
  assert(s9ref);

  nstr_t *s9 =
      nstr_new_sub(
          s9ref,
          1,
          4);
  assert(s9);
  assert(s9->type == NSTR_SUB_T);
  assert(s9->len == 4);
  assert(s9->sub.ref == s9ref);
  assert(s9->sub.offset == 1);
  assert(s9ref->refs == 2);

  nstr_unref(s9ref);
  assert(s9ref->refs == 1);

  nstr_t *s10ref =
      nstr_new_cstr(
          "Xstr10X");
  assert(s10ref);

  nstr_t *s10 =
      nstr_new_sub(
          s10ref,
          1,
          5);

  nstr_unref(s10ref);

  nstr_t *s11 = nstr_new();

  assert(s11);
  assert(s11->type == NSTR_EMPTY_T);
  assert(s11->len == 0);

  nstr_t *s12 = nstr_new_sub(s11, 0, 0);

  assert(s12);
  assert(s12->type == NSTR_EMPTY_T);
  assert(s12->len == 0);

  char *s13c0 = strdup("Xs");
  assert(s13c0);
  nstr_t *s13s0 =
      nstr_new_buffer(
          s13c0, s13c0+1, 1, 3);
  assert(s13s0);

  char *s13c1 = strdup("t");
  assert(s13c0);
  nstr_t *s13s1 =
      nstr_new_buffer(
          s13c1, s13c1, 1, 2);
  assert(s13s1);

  char *s13c2 = strdup("r");
  assert(s13c2);
  nstr_t *s13s2 =
      nstr_new_buffer(
          s13c2, s13c2, 1, 2);
  assert(s13s2);

  char *s13c3 = strdup("1");
  assert(s13c3);
  nstr_t *s13s3 =
      nstr_new_buffer(
          s13c3, s13c3, 1, 2);
  assert(s13s3);

  char *s13c4 = strdup("3");
  assert(s13c4);
  nstr_t *s13s4 =
      nstr_new_buffer(
          s13c4, s13c4, 1, 2);
  assert(s13s4);

  nstr_t *s13 = nstr_concat(s13s0, 0);

  assert(s13);
  assert(s13 == s13s0);

  s13 = nstr_concat(s13s0, s13s1, s13s2, 0);

  assert(s13);
  assert(s13->type == NSTR_BUFFER_T);
  assert(s13 == s13s0);
  assert(s13->len == 3);
  assert(memcmp(s13->buf.str, "str", 3) == 0);

  s13 = nstr_concat(s13, s13s3, s13s4, 0);

  assert(s13);
  assert(s13->type == NSTR_LIST_T);
  assert(s13->len == 5);

  assert(((nstr_t *)(s13->lst[1]))->len == 2);
  assert(((nstr_t *)(s13->lst[1]))->type == NSTR_BUFFER_T);
  assert(memcmp(((nstr_t *)(s13->lst[1]))->buf.str, "13", 2) == 0);

  char *s14c0 = strdup("s");
  assert(s14c0);
  nstr_t *s14s0 =
      nstr_new_buffer(
          s14c0, s14c0, 1, 1);
  assert(s14s0);
  assert(s14s0->type == NSTR_BUFFER_T);

  char *s14c1 = strdup("t");
  assert(s14c1);
  nstr_t *s14s1 =
      nstr_new_buffer(
          s14c1, s14c1, 1, 1);
  assert(s14s1);

  char *s14c2 = strdup("r");
  assert(s14c2);
  nstr_t *s14s2 =
      nstr_new_buffer(
          s14c2, s14c2, 1, 1);
  assert(s14s2);

  char *s14c3 = strdup("1");
  assert(s14c3);
  nstr_t *s14s3 =
      nstr_new_buffer(
          s14c3, s14c3, 1, 1);
  assert(s14s3);

  char *s14c4 = strdup("4");
  assert(s14c4);
  nstr_t *s14s4 =
      nstr_new_buffer(
          s14c4, s14c4, 1, 1);
  assert(s14s4);

  s14s0 = nstr_concat(s14s0, s14s1, 0);
  assert(s14s0);
  assert(s14s0->type == NSTR_LIST_T);

  s14s2 = nstr_concat(s14s2, s14s3, s14s4, 0);
  assert(s14s2);
  assert(s14s2->type == NSTR_LIST_T);

  nstr_t *s14 =
      nstr_concat(s14s0, s14s2, 0);

  assert(s14);
  assert(s14->type == NSTR_LIST_T);
  assert(s14->len = 5);
  assert(s14 == s14s0);

#define ck_str(n) \
    str = nstr_cstr(s ## n); \
    assert(str); \
    assert(strcmp(str, "str" # n) == 0);

  ck_str(1);
  ck_str(2);
  ck_str(3);
  ck_str(4);
  ck_str(5);
  ck_str(6);
  ck_str(7);
  assert(s7->type == NSTR_BUFFER_CSTR_T);
  assert(s7->buf.len == 5);
  assert(strcmp(s7->buf.str, "str7") == 0);

  ck_str(8);
  assert(s8->type == NSTR_BUFFER_CSTR_T);
  assert(strcmp(s8->buf.str, "str8") == 0);

  ck_str(9);
  assert(s9->type == NSTR_SUB_T);

  ck_str(10);
  assert(s10->type == NSTR_BUFFER_CSTR_T);
  assert(s10->len == 5);
  assert(s10->buf.str == s10->buf.start);
  assert(s10->buf.len == s10->len + 1);

  str = nstr_cstr(s11);
  assert(str);
  assert(strcmp(str, "") == 0);

  ck_str(13);
  assert(s13->type == NSTR_BUFFER_CSTR_T);
  assert(s13->len == 5);
  assert(s13->buf.str == s13->buf.start);
  assert(s13->buf.len == s13->len + 1);

  nstr_unref(s1);
  nstr_unref(s2);
  nstr_unref(s3);
  nstr_unref(s4);
  nstr_unref(s5);
  nstr_unref(s6);
  nstr_unref(s7);
  nstr_unref(s8);
  nstr_unref(s9);
  nstr_unref(s10);
  nstr_unref(s11);
  nstr_unref(s12);
  nstr_unref(s13);
  nstr_unref(s14);

  return ret;
}
